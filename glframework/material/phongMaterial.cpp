#include "phongMaterial.h"
#include "../shader.h"
#include "../mesh/mesh.h"
#include "../../application/camera/camera.h"
#include "../light/pointLight.h"
#include <string>

PhongMaterial::PhongMaterial() {
	mType = MaterialType::PhongMaterial;
}

PhongMaterial::~PhongMaterial() {

}

const char* PhongMaterial::getVertexShaderPath() const {
	return "assets/shaders/advanced/phong.vert";
}

const char* PhongMaterial::getFragmentShaderPath() const {
	return "assets/shaders/advanced/phong.frag";
}

void PhongMaterial::applyUniforms(
	Shader* shader,
	Mesh* mesh,
	Camera* camera,
	const std::vector<PointLight*>& pointLights
) {
	shader->setInt("sampler", 0);
	if (mDiffuse) mDiffuse->bind();

	shader->setInt("specularMaskSampler", 1);
	if (mSpecularMask) mSpecularMask->bind();

	shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
	shader->setMatrix4x4("viewMatrix", camera->getViewMatrix());
	shader->setMatrix4x4("projectionMatrix", camera->getProjectionMatrix());

	auto normalMatrix = glm::mat3(glm::transpose(glm::inverse(mesh->getModelMatrix())));
	shader->setMatrix3x3("normalMatrix", normalMatrix);

	shader->setFloat("shiness", mShiness);
	shader->setVector3("cameraPosition", camera->mPosition);

	for (int i = 0; i < pointLights.size(); i++) {
		shader->setVector3("pointLights[" + std::to_string(i) + "].color", pointLights[i]->mColor);
		shader->setVector3("pointLights[" + std::to_string(i) + "].position", pointLights[i]->getPosition());
		shader->setFloat("pointLights[" + std::to_string(i) + "].k2", pointLights[i]->mK2);
		shader->setFloat("pointLights[" + std::to_string(i) + "].k1", pointLights[i]->mK1);
		shader->setFloat("pointLights[" + std::to_string(i) + "].kc", pointLights[i]->mKc);
		shader->setFloat("pointLights[" + std::to_string(i) + "].specularIntensity", pointLights[i]->mSpecularIntensity);
	}
}