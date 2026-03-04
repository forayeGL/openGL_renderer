#include "grassInstanceMaterial.h"
#include "../shader.h"
#include "../mesh/mesh.h"
#include "../../application/camera/camera.h"
#include "../light/pointLight.h"
#include <string>

GrassInstanceMaterial::GrassInstanceMaterial() {
	mType = MaterialType::GrassInstanceMaterial;
}

GrassInstanceMaterial::~GrassInstanceMaterial() {

}

const char* GrassInstanceMaterial::getVertexShaderPath() const {
	return "assets/shaders/grassInstance.vert";
}

const char* GrassInstanceMaterial::getFragmentShaderPath() const {
	return "assets/shaders/grassInstance.frag";
}

void GrassInstanceMaterial::applyUniforms(
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
	shader->setFloat("uvScale", mUVScale);
	shader->setFloat("brightness", mBrightness);

	shader->setFloat("windScale", mWindScale);
	shader->setVector3("windDirection", mWindDirection);
	shader->setFloat("phaseScale", mPhaseScale);

	shader->setInt("cloudMaskSampler", 2);
	if (mCloudMask) mCloudMask->bind();

	shader->setVector3("cloudWhiteColor", mCloudWhiteColor);
	shader->setVector3("cloudBlackColor", mCloudBlackColor);
	shader->setFloat("cloudUVScale", mCloudUVScale);
	shader->setFloat("cloudSpeed", mCloudSpeed);
	shader->setFloat("cloudLerp", mCloudLerp);

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