#include "pbrMaterial.h"
#include "../../shader.h"
#include "../../mesh/mesh.h"
#include "../../../application/camera/camera.h"
#include "../../light/pointLight.h"
#include <string>

PbrMaterial::PbrMaterial() {
	mType = MaterialType::PbrMaterial;
}

PbrMaterial::~PbrMaterial() {

}

const char* PbrMaterial::getVertexShaderPath() const {
	return "assets/shaders/advanced/pbr/pbr.vert";
}

const char* PbrMaterial::getFragmentShaderPath() const {
	return "assets/shaders/advanced/pbr/pbr.frag";
}

void PbrMaterial::applyUniforms(
	Shader* shader,
	Mesh* mesh,
	Camera* camera,
	const std::vector<PointLight*>& pointLights
) {
	shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
	shader->setMatrix4x4("viewMatrix", camera->getViewMatrix());
	shader->setMatrix4x4("projectionMatrix", camera->getProjectionMatrix());
	auto normalMatrix = glm::mat3(glm::transpose(glm::inverse(mesh->getModelMatrix())));
	shader->setMatrix3x3("normalMatrix", normalMatrix);
	shader->setVector3("cameraPosition", camera->mPosition);

	shader->setInt("albedoTex", 0);
	mAlbedo->setUnit(0);
	mAlbedo->bind();

	shader->setInt("normalTex", 1);
	mNormal->setUnit(1);
	mNormal->bind();

	shader->setInt("roughnessTex", 2);
	mRoughness->setUnit(2);
	mRoughness->bind();

	shader->setInt("metallicTex", 3);
	mMetallic->setUnit(3);
	mMetallic->bind();

	shader->setInt("irradianceMap", 4);
	mIrradianceIndirect->setUnit(4);
	mIrradianceIndirect->bind();

	for (int i = 0; i < pointLights.size(); i++) {
		shader->setVector3("pointLights[" + std::to_string(i) + "].color", pointLights[i]->mColor);
		shader->setVector3("pointLights[" + std::to_string(i) + "].position", pointLights[i]->getPosition());
	}
}