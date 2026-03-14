#include "pbrMaterial.h"
#include "../../shader.h"
#include "../../mesh/mesh.h"
#include "../../../application/camera/camera.h"
#include "../../light/pointLight.h"
#include "../../texture.h"
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
	// 变换矩阵
	shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
	shader->setMatrix4x4("viewMatrix", camera->getViewMatrix());
	shader->setMatrix4x4("projectionMatrix", camera->getProjectionMatrix());
	auto normalMatrix = glm::mat3(glm::transpose(glm::inverse(mesh->getModelMatrix())));
	shader->setMatrix3x3("normalMatrix", normalMatrix);

	// 反照率贴图
	shader->setInt("albedoTex", 0);
	if (mAlbedo) {
		mAlbedo->setUnit(0);
		mAlbedo->bind();
	}

	// 法线贴图
	shader->setInt("normalTex", 1);
	shader->setInt("useNormalMap", mUseNormalMap && mNormal ? 1 : 0);
	if (mNormal) {
		mNormal->setUnit(1);
		mNormal->bind();
	}

	// 粗糙度贴图
	shader->setInt("roughnessTex", 2);
	if (mRoughness) {
		mRoughness->setUnit(2);
		mRoughness->bind();
	}

	// 金属度贴图
	shader->setInt("metallicTex", 3);
	if (mMetallic) {
		mMetallic->setUnit(3);
		mMetallic->bind();
	}

	// 环境光遮蔽贴图
	shader->setInt("aoTex", 4);
	if (mAO) {
		mAO->setUnit(4);
		mAO->bind();
	}

	// IBL纹理
	// 使用纹理单元13-15，避免与阴影贴图(unit 6-12)冲突
	// 始终设置sampler uniform指向独立单元，防止samplerCube默认指向
	// unit 0（2D纹理）导致类型不匹配，RenderDoc会因此崩溃
	shader->setInt("irradianceMap", 13);
	shader->setInt("prefilteredMap", 14);
	shader->setInt("brdfLUT", 15);

	// 始终绑定IBL纹理（如果句柄有效），确保samplerCube/sampler2D
	// 类型与实际纹理类型匹配，即使useIBL=false也不会触发RenderDoc校验错误
	if (mIrradianceIndirect) {
		glActiveTexture(GL_TEXTURE13);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceIndirect);
	}
	if (mPrefilteredMap) {
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilteredMap);
	}
	if (mBRDFLUT) {
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_2D, mBRDFLUT);
	}

	shader->setInt("useIBL",
		(mUseIBL && mIrradianceIndirect && mPrefilteredMap && mBRDFLUT) ? 1 : 0);

	// 光源、阴影、环境光、相机位置等通过UBO传递
}