#include "IBLGenerator.h"
#include "../../wrapper/checkError.h"
#include "../../application/tinyexr.h"

#include <iostream>
#include <vector>
#include <cmath>

// 6 capture view matrices — one per cubemap face (+X,-X,+Y,-Y,+Z,-Z)
static glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
static glm::mat4 captureViews[] = {
	glm::lookAt(glm::vec3(0), glm::vec3( 1, 0, 0), glm::vec3(0,-1, 0)),
	glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
	glm::lookAt(glm::vec3(0), glm::vec3( 0, 1, 0), glm::vec3(0, 0, 1)),
	glm::lookAt(glm::vec3(0), glm::vec3( 0,-1, 0), glm::vec3(0, 0,-1)),
	glm::lookAt(glm::vec3(0), glm::vec3( 0, 0, 1), glm::vec3(0,-1, 0)),
	glm::lookAt(glm::vec3(0), glm::vec3( 0, 0,-1), glm::vec3(0,-1, 0)),
};

IBLGenerator::~IBLGenerator() {
	// mCubeGeo is owned by Geometry (static factory), don't delete
}

// ─── Private helpers ──────────────────────────────────────────────────────────

void IBLGenerator::ensureCubeGeometry() {
	if (!mCubeGeo) {
		mCubeGeo = Geometry::createBox(1.0f);
	}
}

GLuint IBLGenerator::createCubemapTexture(int faceSize, bool mipmapped) {
	GLuint cubemap;
	glGenTextures(1, &cubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);
	for (int i = 0; i < 6; i++) {
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
			faceSize, faceSize, 0, GL_RGB, GL_FLOAT, nullptr);
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	// Mipmapping is required for prefiltering step to avoid fireflies, 
	// but not needed for simple env map or irradiance map.
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER,
		mipmapped ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	return cubemap;
}

void IBLGenerator::createCaptureBuffer(int size, GLuint& fbo, GLuint& rbo) {
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &rbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, size, size);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
}

void IBLGenerator::renderCubeFaces(Shader* shader, GLuint fbo, GLuint cubemap, int faceSize, int mipLevel) {
	ensureCubeGeometry();
	glViewport(0, 0, faceSize, faceSize);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	for (int i = 0; i < 6; i++) {
		shader->setMatrix4x4("viewMatrix", captureViews[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, cubemap, mipLevel);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBindVertexArray(mCubeGeo->getVao());
		glDrawElements(GL_TRIANGLES, mCubeGeo->getIndicesCount(), GL_UNSIGNED_INT, 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// ─── Public API ───────────────────────────────────────────────────────────────

GLuint IBLGenerator::equirectToCubemap(const std::string& exrPath, int faceSize) {
	// Load equirectangular HDR/EXR
	int width = 0, height = 0;
	float* data = nullptr;
	const char* err = nullptr;
	int ret = LoadEXR(&data, &width, &height, exrPath.c_str(), &err);
	if (ret != TINYEXR_SUCCESS || !data) {
		std::cerr << "[IBL] Failed to load EXR: " << exrPath;
		if (err) std::cerr << " (" << err << ")";
		std::cerr << std::endl;
		return 0;
	}

	// Flip Y (OpenGL vs EXR convention)
	const int nc = 4;
	for (int y = 0; y < height / 2; ++y) {
		int oy = height - y - 1;
		for (int x = 0; x < width * nc; ++x)
			std::swap(data[y * width * nc + x], data[oy * width * nc + x]);
	}

	GLuint hdrTex;
	glGenTextures(1, &hdrTex);
	glBindTexture(GL_TEXTURE_2D, hdrTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGBA, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	free(data);

	// Create env cubemap (mipmapped — required for firefly-free prefiltering)
	GLuint envCubemap = createCubemapTexture(faceSize, true);

	GLuint captureFBO, captureRBO;
	createCaptureBuffer(faceSize, captureFBO, captureRBO);

	Shader equirectShader(
		"assets/shaders/ibl/equirect_to_cube.vert",
		"assets/shaders/ibl/equirect_to_cube.frag"
	);
	equirectShader.begin();
	equirectShader.setInt("equirectangularMap", 0);
	equirectShader.setMatrix4x4("projectionMatrix", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, hdrTex);

	renderCubeFaces(&equirectShader, captureFBO, envCubemap, faceSize);

	// Generate full mip chain from the rendered mip-0 data
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	glDeleteTextures(1, &hdrTex);
	glDeleteRenderbuffers(1, &captureRBO);
	glDeleteFramebuffers(1, &captureFBO);

	return envCubemap;
}

GLuint IBLGenerator::loadCubemapFromExr(const std::vector<std::string>& facePaths) {
	if (facePaths.size() != 6) {
		std::cerr << "[IBL] loadCubemapFromExr: expected 6 face paths, got "
				  << facePaths.size() << std::endl;
		return 0;
	}

	GLuint cubemap;
	glGenTextures(1, &cubemap);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

	for (int i = 0; i < 6; i++) {
		int width = 0, height = 0;
		float* data = nullptr;
		const char* err = nullptr;
		int ret = LoadEXR(&data, &width, &height, facePaths[i].c_str(), &err);
		if (ret != TINYEXR_SUCCESS || !data) {
			std::cerr << "[IBL] Failed to load face EXR: " << facePaths[i];
			if (err) std::cerr << " (" << err << ")";
			std::cerr << std::endl;
			glDeleteTextures(1, &cubemap);
			return 0;
		}

		// Flip Y
		const int nc = 4;
		for (int y = 0; y < height / 2; ++y) {
			int oy = height - y - 1;
			for (int x = 0; x < width * nc; ++x)
				std::swap(data[y * width * nc + x], data[oy * width * nc + x]);
		}

		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
			width, height, 0, GL_RGBA, GL_FLOAT, data);
		free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	return cubemap;
}

GLuint IBLGenerator::generateIrradiance(GLuint envCubemap, int faceSize) {
	GLuint irradianceMap = createCubemapTexture(faceSize, false);

	GLuint captureFBO, captureRBO;
	createCaptureBuffer(faceSize, captureFBO, captureRBO);

	Shader irradianceShader(
		"assets/shaders/ibl/equirect_to_cube.vert",
		"assets/shaders/ibl/irradiance.frag"
	);
	irradianceShader.begin();
	irradianceShader.setInt("environmentMap", 0);
	irradianceShader.setMatrix4x4("projectionMatrix", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	renderCubeFaces(&irradianceShader, captureFBO, irradianceMap, faceSize);

	glDeleteRenderbuffers(1, &captureRBO);
	glDeleteFramebuffers(1, &captureFBO);

	return irradianceMap;
}

GLuint IBLGenerator::generatePrefilteredEnvMap(GLuint envCubemap, int faceSize, int maxMipLevels) {
	// Each mip level represents a different roughness: mip 0 = smooth, mip N-1 = rough.
	GLuint prefilterMap = createCubemapTexture(faceSize, true);

	// Allocate the full mip chain storage before rendering into individual levels
	glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	GLuint captureFBO, captureRBO;
	createCaptureBuffer(faceSize, captureFBO, captureRBO);

	Shader prefilterShader(
		"assets/shaders/ibl/equirect_to_cube.vert",
		"assets/shaders/ibl/prefilter.frag"
	);
	prefilterShader.begin();
	prefilterShader.setInt("environmentMap", 0);
	prefilterShader.setMatrix4x4("projectionMatrix", captureProjection);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

	ensureCubeGeometry();
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

	for (int mip = 0; mip < maxMipLevels; ++mip) {
		// Each mip is half the size of the previous
		int mipSize = static_cast<int>(faceSize * std::pow(0.5f, static_cast<float>(mip)));
		float roughness = static_cast<float>(mip) / static_cast<float>(maxMipLevels - 1);

		// Resize the depth RBO to match this mip level
		glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipSize, mipSize);
		glViewport(0, 0, mipSize, mipSize);

		prefilterShader.setFloat("roughness", roughness);

		for (int i = 0; i < 6; ++i) {
			prefilterShader.setMatrix4x4("viewMatrix", captureViews[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindVertexArray(mCubeGeo->getVao());
			glDrawElements(GL_TRIANGLES, mCubeGeo->getIndicesCount(), GL_UNSIGNED_INT, 0);
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteRenderbuffers(1, &captureRBO);
	glDeleteFramebuffers(1, &captureFBO);

	return prefilterMap;
}

GLuint IBLGenerator::generateBRDFLUT(int resolution) {
	GLuint brdfLUT;
	glGenTextures(1, &brdfLUT);
	glBindTexture(GL_TEXTURE_2D, brdfLUT);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, resolution, resolution, 0, GL_RG, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	GLuint captureFBO;
	glGenFramebuffers(1, &captureFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUT, 0);

	Shader brdfShader(
		"assets/shaders/ibl/brdf_lut.vert",
		"assets/shaders/ibl/brdf_lut.frag"
	);
	brdfShader.begin();
	glViewport(0, 0, resolution, resolution);
	glClear(GL_COLOR_BUFFER_BIT);

	Geometry* quad = Geometry::createScreenPlane();
	glBindVertexArray(quad->getVao());
	glDrawElements(GL_TRIANGLES, quad->getIndicesCount(), GL_UNSIGNED_INT, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDeleteFramebuffers(1, &captureFBO);

	return brdfLUT;
}

bool IBLGenerator::saveCubemapToExr(GLuint cubemap, int faceSize, const std::string& prefix) {
	const int channels = 4;
	std::vector<float> pixels(faceSize * faceSize * channels);

	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemap);

	const char* faceNames[] = { "px", "nx", "py", "ny", "pz", "nz" };
	for (int i = 0; i < 6; i++) {
		glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA, GL_FLOAT, pixels.data());

		// Flip Y for EXR convention
		for (int y = 0; y < faceSize / 2; ++y) {
			int oy = faceSize - y - 1;
			for (int x = 0; x < faceSize * channels; ++x) {
				std::swap(pixels[y * faceSize * channels + x],
						  pixels[oy * faceSize * channels + x]);
			}
		}

		std::string path = prefix + "_" + faceNames[i] + ".exr";
		const char* err = nullptr;
		int ret = SaveEXR(pixels.data(), faceSize, faceSize, channels, 0, path.c_str(), &err);
		if (ret != TINYEXR_SUCCESS) {
			std::cerr << "[IBL] Failed to save " << path;
			if (err) std::cerr << " (" << err << ")";
			std::cerr << std::endl;
			return false;
		}
		std::cout << "[IBL] Saved: " << path << std::endl;
	}
	return true;
}

bool IBLGenerator::saveTexture2DToExr(GLuint texture, int width, int height, const std::string& path) {
	const int channels = 4;
	std::vector<float> pixels(width * height * channels);

	glBindTexture(GL_TEXTURE_2D, texture);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data());

	// Flip Y
	for (int y = 0; y < height / 2; ++y) {
		int oy = height - y - 1;
		for (int x = 0; x < width * channels; ++x) {
			std::swap(pixels[y * width * channels + x],
					  pixels[oy * width * channels + x]);
		}
	}

	const char* err = nullptr;
	int ret = SaveEXR(pixels.data(), width, height, channels, 0, path.c_str(), &err);
	if (ret != TINYEXR_SUCCESS) {
		std::cerr << "[IBL] Failed to save " << path;
		if (err) std::cerr << " (" << err << ")";
		std::cerr << std::endl;
		return false;
	}
	std::cout << "[IBL] Saved: " << path << std::endl;
	return true;
}

