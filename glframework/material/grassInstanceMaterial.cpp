#include "grassInstanceMaterial.h"
#include "../shader.h"
#include "../mesh/mesh.h"
#include "../../application/camera/camera.h"
#include "../light/pointLight.h"
#include <string>

GrassInstanceMaterial::GrassInstanceMaterial() {
	mType = MaterialType::GrassInstanceMaterial;
   mInstanced = true;
	mFaceCulling = false;
	mBlend = false;
	mDepthWrite = true;
}

GrassInstanceMaterial::~GrassInstanceMaterial() {

}

const char* GrassInstanceMaterial::getVertexShaderPath() const {
 return "assets/shaders/advanced/grass/grass_pbr_instanced.vert";
}

const char* GrassInstanceMaterial::getFragmentShaderPath() const {
 return "assets/shaders/advanced/grass/grass_pbr_instanced.frag";
}

void GrassInstanceMaterial::applyUniforms(
	Shader* shader,
	Mesh* mesh,
	Camera* camera,
	const std::vector<PointLight*>& pointLights
) {
    if (!mAlbedo && mDiffuse) {
		mAlbedo = mDiffuse;
	}
	if (!mOpacity && mOpacityMask) {
		mOpacity = mOpacityMask;
	}

	shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
	shader->setMatrix4x4("viewMatrix", camera->getViewMatrix());
	shader->setMatrix4x4("projectionMatrix", camera->getProjectionMatrix());

	auto normalMatrix = glm::mat3(glm::transpose(glm::inverse(mesh->getModelMatrix())));
	shader->setMatrix3x3("normalMatrix", normalMatrix);
	shader->setVector3("cameraPosition", camera->mPosition);
	shader->setFloat("time", static_cast<float>(glfwGetTime()));

	shader->setInt("albedoTex", 0);
	shader->setInt("normalTex", 1);
	shader->setInt("roughnessTex", 2);
	shader->setInt("metallicTex", 3);
	shader->setInt("aoTex", 4);
	shader->setInt("opacityTex", 5);
	shader->setInt("thicknessTex", 6);

	if (mAlbedo) {
		mAlbedo->setUnit(0);
		mAlbedo->bind();
	}
	if (mNormal) {
		mNormal->setUnit(1);
		mNormal->bind();
	}
	if (mRoughness) {
		mRoughness->setUnit(2);
		mRoughness->bind();
	}
	if (mMetallic) {
		mMetallic->setUnit(3);
		mMetallic->bind();
	}
	if (mAO) {
		mAO->setUnit(4);
		mAO->bind();
	}
	if (mOpacity) {
		mOpacity->setUnit(5);
		mOpacity->bind();
	}
	if (mThickness) {
		mThickness->setUnit(6);
		mThickness->bind();
	}

	shader->setInt("useAlbedoMap", (mUseAlbedoMap && mAlbedo) ? 1 : 0);
	shader->setInt("useNormalMap", (mUseNormalMap && mNormal) ? 1 : 0);
	shader->setInt("useRoughnessMap", (mUseRoughnessMap && mRoughness) ? 1 : 0);
	shader->setInt("useMetallicMap", (mUseMetallicMap && mMetallic) ? 1 : 0);
	shader->setInt("useAOMap", (mUseAOMap && mAO) ? 1 : 0);
	shader->setInt("useOpacityMap", (mUseOpacityMap && mOpacity) ? 1 : 0);
	shader->setInt("useThicknessMap", (mUseThicknessMap && mThickness) ? 1 : 0);
	shader->setInt("useTransmission", mUseTransmission ? 1 : 0);
	shader->setInt("useIBL", (mUseIBL && mIrradianceIndirect && mPrefilteredMap && mBRDFLUT) ? 1 : 0);

	shader->setVector3("albedoValue", mAlbedoValue);
	shader->setFloat("roughnessValue", mRoughnessValue);
	shader->setFloat("metallicValue", mMetallicValue);
	shader->setFloat("aoValue", mAOValue);
	shader->setFloat("opacityValue", mOpacityValue);
	shader->setFloat("thicknessValue", mThicknessValue);
	shader->setFloat("specularBoost", mSpecularBoost);
	shader->setVector3("subsurfaceColor", mSubsurfaceColor);
	shader->setFloat("transmissionStrength", mTransmissionStrength);
	shader->setFloat("alphaCutoff", mAlphaCutoff);
	shader->setFloat("uvScale", mUVScale);
	shader->setFloat("brightness", mBrightness);

	shader->setVector3("windDirection", mWindDirection);
	shader->setFloat("windAmplitude", mWindAmplitude);
	shader->setFloat("windFrequency", mWindFrequency);
	shader->setFloat("windGustAmplitude", mWindGustAmplitude);
	shader->setFloat("windGustFrequency", mWindGustFrequency);
	shader->setFloat("phaseScale", mPhaseScale);

   shader->setInt("irradianceMap", 14);
	shader->setInt("prefilteredMap", 15);
	shader->setInt("brdfLUT", 16);
	if (mIrradianceIndirect) {
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceIndirect);
	}
	if (mPrefilteredMap) {
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilteredMap);
	}
	if (mBRDFLUT) {
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, mBRDFLUT);
	}
}