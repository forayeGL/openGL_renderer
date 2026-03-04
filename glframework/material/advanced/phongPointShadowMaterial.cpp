#include "phongPointShadowMaterial.h"
#include "../../shader.h"
#include "../../mesh/mesh.h"
#include "../../../application/camera/camera.h"
#include "../../light/pointLight.h"
#include <string>

PhongPointShadowMaterial::PhongPointShadowMaterial() {
	mType = MaterialType::PhongPointShadowMaterial;
}

PhongPointShadowMaterial::~PhongPointShadowMaterial() {

}

const char* PhongPointShadowMaterial::getVertexShaderPath() const {
	return "assets/shaders/advanced/phongPointShadow.vert";
}

const char* PhongPointShadowMaterial::getFragmentShaderPath() const {
	return "assets/shaders/advanced/phongPointShadow.frag";
}

void PhongPointShadowMaterial::applyUniforms(
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

	shader->setInt("sampler", 0);
	if (mDiffuse) mDiffuse->bind();

	shader->setInt("specularMaskSampler", 1);
	if (mSpecularMask) mSpecularMask->bind();

	shader->setFloat("shiness", mShiness);
	shader->setVector3("cameraPosition", camera->mPosition);
	shader->setVector3("ambientColor", 0.15f, 0.15f, 0.15f);
	shader->setFloat("opacity", mOpacity);

	if (!pointLights.empty()) {
		shader->setVector3("pointLight.color", pointLights[0]->mColor);
		shader->setVector3("pointLight.position", pointLights[0]->getPosition());
		shader->setFloat("pointLight.k2", pointLights[0]->mK2);
		shader->setFloat("pointLight.k1", pointLights[0]->mK1);
		shader->setFloat("pointLight.kc", pointLights[0]->mKc);
		shader->setFloat("pointLight.specularIntensity", pointLights[0]->mSpecularIntensity);
	}
}