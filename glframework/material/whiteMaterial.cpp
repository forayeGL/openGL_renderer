#include "whiteMaterial.h"
#include "../shader.h"
#include "../mesh/mesh.h"
#include "../../application/camera/camera.h"

WhiteMaterial::WhiteMaterial() {
	mType = MaterialType::WhiteMaterial;
}

WhiteMaterial::~WhiteMaterial() {

}

const char* WhiteMaterial::getVertexShaderPath() const {
	return "assets/shaders/white.vert";
}

const char* WhiteMaterial::getFragmentShaderPath() const {
	return "assets/shaders/white.frag";
}

void WhiteMaterial::applyUniforms(
	Shader* shader,
	Mesh* mesh,
	Camera* camera,
	const std::vector<PointLight*>& pointLights
) {
	shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
	shader->setMatrix4x4("viewMatrix", camera->getViewMatrix());
	shader->setMatrix4x4("projectionMatrix", camera->getProjectionMatrix());
}