#include "cubeMaterial.h"
#include "../shader.h"
#include "../mesh/mesh.h"
#include "../../application/camera/camera.h"

CubeMaterial::CubeMaterial() {
	mType = MaterialType::CubeMaterial;
}

CubeMaterial::~CubeMaterial() {

}

const char* CubeMaterial::getVertexShaderPath() const {
	return "assets/shaders/cube.vert";
}

const char* CubeMaterial::getFragmentShaderPath() const {
	return "assets/shaders/cube.frag";
}

void CubeMaterial::applyUniforms(
	Shader* shader,
	Mesh* mesh,
	Camera* camera,
	const std::vector<PointLight*>& pointLights
) {
	mesh->setPosition(camera->mPosition);
	shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
	shader->setMatrix4x4("viewMatrix", camera->getViewMatrix());
	shader->setMatrix4x4("projectionMatrix", camera->getProjectionMatrix());

	shader->setInt("sphericalSampler", 0);
	mDiffuse->bind();
}