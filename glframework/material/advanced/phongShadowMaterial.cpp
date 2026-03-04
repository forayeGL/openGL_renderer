#include "phongShadowMaterial.h"
#include "../../shader.h"
#include "../../mesh/mesh.h"
#include "../../../application/camera/camera.h"
#include "../../light/pointLight.h"
#include <string>

PhongShadowMaterial::PhongShadowMaterial() {
	mType = MaterialType::PhongShadowMaterial;
}

PhongShadowMaterial::~PhongShadowMaterial() {

}

const char* PhongShadowMaterial::getVertexShaderPath() const {
	return "assets/shaders/advanced/phongShadow.vert";
}

const char* PhongShadowMaterial::getFragmentShaderPath() const {
	return "assets/shaders/advanced/phongShadow.frag";
}

void PhongShadowMaterial::applyUniforms(
	Shader* shader,
	Mesh* mesh,
	Camera* camera,
	const std::vector<PointLight*>& pointLights
) {
	// Per-object transforms
	shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
	shader->setMatrix4x4("viewMatrix", camera->getViewMatrix());
	shader->setMatrix4x4("projectionMatrix", camera->getProjectionMatrix());

	auto normalMatrix = glm::mat3(glm::transpose(glm::inverse(mesh->getModelMatrix())));
	shader->setMatrix3x3("normalMatrix", normalMatrix);

	// Per-object textures
	shader->setInt("sampler", 0);
	if (mDiffuse) mDiffuse->bind();

	shader->setInt("specularMaskSampler", 1);
	if (mSpecularMask) mSpecularMask->bind();

	// Per-material params
	shader->setFloat("shiness", mShiness);

	// Lights, shadow, ambient, camera, opacity all come from UBOs
}