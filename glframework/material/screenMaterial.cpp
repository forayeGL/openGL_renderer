#include "screenMaterial.h"
#include "../shader.h"
#include "../mesh/mesh.h"
#include "../../application/camera/camera.h"

ScreenMaterial::ScreenMaterial() {
	mType = MaterialType::ScreenMaterial;
}

ScreenMaterial::~ScreenMaterial() {

}

const char* ScreenMaterial::getVertexShaderPath() const {
	return "assets/shaders/screen.vert";
}

const char* ScreenMaterial::getFragmentShaderPath() const {
	return "assets/shaders/screen.frag";
}

void ScreenMaterial::applyUniforms(
	Shader* shader,
	Mesh* mesh,
	Camera* camera,
	const std::vector<PointLight*>& pointLights
) {
	shader->setInt("screenTexSampler", 0);

	shader->setFloat("texWidth", 1600);
	shader->setFloat("texHeight", 1200);

	mScreenTexture->bind();
	shader->setFloat("exposure", mExposure);
}