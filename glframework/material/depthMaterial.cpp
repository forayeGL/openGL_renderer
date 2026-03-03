#include "depthMaterial.h"
#include "../shader.h"
#include "../mesh/mesh.h"
#include "../../application/camera/camera.h"

DepthMaterial::DepthMaterial() {
	mType = MaterialType::DepthMaterial;
}

DepthMaterial::~DepthMaterial() {

}

const char* DepthMaterial::getVertexShaderPath() const {
	return "assets/shaders/depth.vert";
}

const char* DepthMaterial::getFragmentShaderPath() const {
	return "assets/shaders/depth.frag";
}

void DepthMaterial::applyUniforms(
	Shader* shader,
	Mesh* mesh,
	Camera* camera,
	const std::vector<PointLight*>& pointLights
) {
	shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
	shader->setMatrix4x4("viewMatrix", camera->getViewMatrix());
	shader->setMatrix4x4("projectionMatrix", camera->getProjectionMatrix());
}