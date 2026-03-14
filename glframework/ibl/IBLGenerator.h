#pragma once
#include "../core.h"
#include "../geometry.h"
#include "../shader.h"
#include <string>
#include <vector>

// IBLGenerator: standalone module for baking IBL resources.
// Generates: environment cubemap (from equirectangular or cubemap faces),
// irradiance cubemap, prefiltered env map, and BRDF LUT.
// All GPU operations require a valid GL context.
class IBLGenerator {
public:
	IBLGenerator() = default;
	~IBLGenerator();

	// Non-copyable
	IBLGenerator(const IBLGenerator&) = delete;
	IBLGenerator& operator=(const IBLGenerator&) = delete;

	// Convert equirectangular HDR/EXR to mipmapped environment cubemap.
	GLuint equirectToCubemap(const std::string& exrPath, int faceSize = 512);

	// Load 6 face EXR files into a mipmapped environment cubemap.
	// facePaths order: +X, -X, +Y, -Y, +Z, -Z
	GLuint loadCubemapFromExr(const std::vector<std::string>& facePaths);

	// Convolve environment cubemap into diffuse irradiance cubemap.
	GLuint generateIrradiance(GLuint envCubemap, int faceSize = 32);

	// Generate prefiltered environment cubemap for specular IBL.
	// Each mip level stores a GGX-convolved result for a different roughness value.
	GLuint generatePrefilteredEnvMap(GLuint envCubemap, int faceSize = 128, int maxMipLevels = 5);

	// Generate BRDF integration LUT (2D RG16F texture).
	GLuint generateBRDFLUT(int resolution = 512);

	// Save a cubemap's 6 faces as individual .exr files.
	// Files: <prefix>_px.exr, _nx.exr, _py.exr, _ny.exr, _pz.exr, _nz.exr
	static bool saveCubemapToExr(GLuint cubemap, int faceSize, const std::string& prefix);

	// Save a 2D texture as a single .exr file.
	static bool saveTexture2DToExr(GLuint texture, int width, int height, const std::string& path);

private:
	void ensureCubeGeometry();

	// Render all 6 faces of a cubemap into fbo at the given mip level.
	// The shader must already be active (begin() called) and projectionMatrix set.
	void renderCubeFaces(Shader* shader, GLuint fbo, GLuint cubemap, int faceSize, int mipLevel = 0);

	// Create an RGB16F cubemap texture with proper wrap/filter parameters.
	// If mipmapped, sets GL_LINEAR_MIPMAP_LINEAR; caller must call glGenerateMipmap when ready.
	GLuint createCubemapTexture(int faceSize, bool mipmapped = false);

	// Create a capture FBO with a depth RBO of the given square size.
	void createCaptureBuffer(int size, GLuint& fbo, GLuint& rbo);

	Geometry* mCubeGeo{ nullptr };
};
