// iblBaker: standalone offline tool for baking IBL resources.
//
// Usage:
//   iblBaker <input.exr> <output_dir> [cubemap_size] [irradiance_size] [brdf_size]
//
// Outputs:
//   <output_dir>/env_cubemap_px.exr .. _nz.exr   (environment cubemap)
//   <output_dir>/irradiance_px.exr  .. _nz.exr   (diffuse irradiance cubemap)
//   <output_dir>/brdf_lut.exr                     (BRDF integration LUT)
//
// Requires an OpenGL 4.6 context (created headless via GLFW).

#include <iostream>
#include <string>
#include <filesystem>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "glframework/ibl/IBLGenerator.h"

static void glfwErrorCb(int error, const char* desc) {
	std::cerr << "[GLFW] Error " << error << ": " << desc << std::endl;
}

int main(int argc, char* argv[]) {
	/*if (argc < 3) {
		std::cout << "Usage: iblBaker <input.exr> <output_dir> [cubemap_size=512] [irradiance_size=32] [brdf_size=512]" << std::endl;
		return 1;
	}*/

	/*std::string inputExr  = argv[1];
	std::string outputDir = argv[2];*/
	std::string inputExr = R"(E:\project\openGLRenderer\openGL_renderer\assets\textures\pbr\bryanston_park_sunrise_2k.exr)";
	std::string outputDir = ".\IBLTexture";
	int cubemapSize    = argc > 3 ? std::atoi(argv[3]) : 512;
	int irradianceSize = argc > 4 ? std::atoi(argv[4]) : 32;
	int brdfSize       = argc > 5 ? std::atoi(argv[5]) : 512;

	// Create output directory if it doesn't exist
	std::filesystem::create_directories(outputDir);

	// Initialize GLFW + headless GL context
	glfwSetErrorCallback(glfwErrorCb);
	if (!glfwInit()) {
		std::cerr << "[iblBaker] Failed to init GLFW" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // headless

	GLFWwindow* window = glfwCreateWindow(64, 64, "iblBaker", nullptr, nullptr);
	if (!window) {
		std::cerr << "[iblBaker] Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "[iblBaker] Failed to init GLAD" << std::endl;
		glfwTerminate();
		return -1;
	}

	std::cout << "[iblBaker] GL context ready: " << glGetString(GL_RENDERER) << std::endl;
	std::cout << "[iblBaker] Input:  " << inputExr << std::endl;
	std::cout << "[iblBaker] Output: " << outputDir << std::endl;
	std::cout << "[iblBaker] Cubemap size:    " << cubemapSize << std::endl;
	std::cout << "[iblBaker] Irradiance size: " << irradianceSize << std::endl;
	std::cout << "[iblBaker] BRDF LUT size:   " << brdfSize << std::endl;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	IBLGenerator gen;

	// Step 1: Equirect → Cubemap
	std::cout << "\n[iblBaker] Converting equirectangular to cubemap..." << std::endl;
	GLuint envCubemap = gen.equirectToCubemap(inputExr, cubemapSize);
	if (!envCubemap) {
		std::cerr << "[iblBaker] Failed to generate environment cubemap." << std::endl;
		glfwTerminate();
		return -1;
	}
	IBLGenerator::saveCubemapToExr(envCubemap, cubemapSize, outputDir + "/env_cubemap");

	// Step 2: Irradiance convolution
	std::cout << "\n[iblBaker] Generating irradiance cubemap..." << std::endl;
	GLuint irradiance = gen.generateIrradiance(envCubemap, irradianceSize);
	IBLGenerator::saveCubemapToExr(irradiance, irradianceSize, outputDir + "/irradiance");

	// Step 3: BRDF LUT
	std::cout << "\n[iblBaker] Generating BRDF LUT..." << std::endl;
	GLuint brdfLUT = gen.generateBRDFLUT(brdfSize);
	IBLGenerator::saveTexture2DToExr(brdfLUT, brdfSize, brdfSize, outputDir + "/brdf_lut");

	// Cleanup
	glDeleteTextures(1, &envCubemap);
	glDeleteTextures(1, &irradiance);
	glDeleteTextures(1, &brdfLUT);

	glfwDestroyWindow(window);
	glfwTerminate();

	std::cout << "\n[iblBaker] Done!" << std::endl;
	return 0;
}
