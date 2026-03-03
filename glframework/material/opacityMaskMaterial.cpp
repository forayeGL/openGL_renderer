#include "opacityMaskMaterial.h"
#include "../shader.h"
#include "../mesh/mesh.h"
#include "../../application/camera/camera.h"

OpacityMaskMaterial::OpacityMaskMaterial() {
	mType = MaterialType::OpacityMaskMaterial;
}

OpacityMaskMaterial::~OpacityMaskMaterial() {

}

const char* OpacityMaskMaterial::getVertexShaderPath() const {
	return "assets/shaders/phongOpacityMask.vert";
}

const char* OpacityMaskMaterial::getFragmentShaderPath() const {
	return "assets/shaders/phongOpacityMask.frag";
}

void OpacityMaskMaterial::applyUniforms(
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

	shader->setInt("opacityMaskSampler", 1);
	if (mOpacityMask) mOpacityMask->bind();

	shader->setFloat("shiness", mShiness);
}