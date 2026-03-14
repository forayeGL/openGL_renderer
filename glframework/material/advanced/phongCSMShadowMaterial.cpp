#include "phongCSMShadowMaterial.h"
#include "../../shader.h"
#include "../../mesh/mesh.h"
#include "../../../application/camera/camera.h"
#include "../../light/pointLight.h"
#include <string>

PhongCSMShadowMaterial::PhongCSMShadowMaterial() {
	mType = MaterialType::PhongCSMShadowMaterial;
}

PhongCSMShadowMaterial::~PhongCSMShadowMaterial() {

}

const char* PhongCSMShadowMaterial::getVertexShaderPath() const {
    return "assets/shaders/shadow/phongCSMShadow.vert";
}

const char* PhongCSMShadowMaterial::getFragmentShaderPath() const {
    return "assets/shaders/shadow/phongCSMShadow.frag";
}

void PhongCSMShadowMaterial::applyUniforms(
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

	// Lights, shadow, ambient, camera, opacity all come from UBOs
}