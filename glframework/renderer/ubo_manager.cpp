#include "ubo_manager.h"
#include "../light/shadow/directionalLightShadow.h"
#include "../light/shadow/pointLightShadow.h"
#include "../../application/camera/orthographicCamera.h"
#include <cstring>
#include <algorithm>
#include <cmath>

namespace {
	constexpr float kMinAttenuation = 1e-4f;
	constexpr float kRangeCutoffLuminance = 0.02f;

	float computePointLightEffectiveRange(const PointLight* pl) {
		if (!pl) return 0.0f;

		if (pl->mRange > 0.0f) {
			return pl->mRange;
		}

		const float lightStrength = std::max({ pl->mColor.r, pl->mColor.g, pl->mColor.b, 0.0f });
		if (lightStrength <= 0.0f) {
			return 0.0f;
		}

		const float k2 = std::max(pl->mK2, 0.0f);
		const float k1 = std::max(pl->mK1, 0.0f);
		const float kc = std::max(pl->mKc, kMinAttenuation);

		// Solve: lightStrength / (k2*d^2 + k1*d + kc) = cutoff
		// => k2*d^2 + k1*d + kc - lightStrength/cutoff = 0
		const float c = kc - lightStrength / kRangeCutoffLuminance;

		if (k2 > kMinAttenuation) {
			const float discriminant = k1 * k1 - 4.0f * k2 * c;
			if (discriminant <= 0.0f) {
				return 0.0f;
			}
			const float radius = (-k1 + std::sqrt(discriminant)) / (2.0f * k2);
			return std::max(radius, 0.0f);
		}

		if (k1 > kMinAttenuation) {
			const float radius = -c / k1;
			return std::max(radius, 0.0f);
		}

		return 0.0f;
	}
}

UBOManager::UBOManager() {
}

UBOManager::~UBOManager() {
	if (mLightUBO)    glDeleteBuffers(1, &mLightUBO);
	if (mShadowUBO)   glDeleteBuffers(1, &mShadowUBO);
	if (mSettingsUBO) glDeleteBuffers(1, &mSettingsUBO);
}

void UBOManager::init() {
	glGenBuffers(1, &mLightUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mLightUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(LightUBOData), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_LIGHTS, mLightUBO);

	glGenBuffers(1, &mShadowUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mShadowUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShadowUBOData), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_SHADOW, mShadowUBO);

	glGenBuffers(1, &mSettingsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, mSettingsUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(RenderSettingsUBOData), nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_RENDER_SETTINGS, mSettingsUBO);

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UBOManager::updateLights(
	DirectionalLight* dirLight,
	const std::vector<PointLight*>& pointLights
) {
	std::memset(&mLightData, 0, sizeof(mLightData));

	if (dirLight) {
		mLightData.dirLight.direction = glm::vec4(dirLight->getDirection(), 0.0f);
		mLightData.dirLight.color = glm::vec4(dirLight->mColor, dirLight->mIntensity);
		mLightData.dirLight.specularPad = glm::vec4(dirLight->mSpecularIntensity, 0.0f, 0.0f, 0.0f);
	}

	int count = std::min((int)pointLights.size(), MAX_POINT_LIGHTS);
	mLightData.counts = glm::ivec4(count, 0, 0, 0);

	for (int i = 0; i < count; i++) {
		auto* pl = pointLights[i];
		mLightData.pointLights[i].position = glm::vec4(pl->getPosition(), 0.0f);
		mLightData.pointLights[i].color = glm::vec4(pl->mColor, 0.0f);
		mLightData.pointLights[i].attenuation = glm::vec4(
			pl->mSpecularIntensity, pl->mK2, pl->mK1, pl->mKc
		);
       mLightData.pointLights[i].params = glm::vec4(computePointLightEffectiveRange(pl), 0.0f, 0.0f, 0.0f);
	}

	glBindBuffer(GL_UNIFORM_BUFFER, mLightUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(LightUBOData), &mLightData);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_LIGHTS, mLightUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UBOManager::updateShadow(
	DirectionalLight* dirLight,
	const std::vector<PointLight*>& pointLights
) {
	std::memset(&mShadowData, 0, sizeof(mShadowData));

	if (dirLight && dirLight->mShadow) {
		auto* shadow = static_cast<DirectionalLightShadow*>(dirLight->mShadow);
		mShadowData.lightMatrix = shadow->getLightMatrix(dirLight->getModelMatrix());
		mShadowData.lightViewMatrix = glm::inverse(dirLight->getModelMatrix());
		mShadowData.params1 = glm::vec4(
			shadow->mBias, shadow->mPcfRadius,
			shadow->mDiskTightness, shadow->mLightSize
		);

		float nearPlane = 0.0f;
		float frustum = 100.0f;
		if (shadow->mCamera) {
			nearPlane = shadow->mCamera->mNear;
			auto* ortho = dynamic_cast<OrthographicCamera*>(shadow->mCamera);
			if (ortho) {
				frustum = ortho->mR - ortho->mL;
			}
		}
		mShadowData.params2 = glm::vec4(nearPlane, frustum, 0.0f, 0.0f);
		mShadowData.flags.x = 1; // hasDirShadow
	}

	int count = std::min((int)pointLights.size(), MAX_POINT_SHADOW);
	mShadowData.flags.y = count; // numPointShadows

	// Pack far values into two vec4s (4 floats each)
	for (int i = 0; i < count && i < 4; i++) {
		if (pointLights[i]->mShadow && pointLights[i]->mShadow->mCamera) {
			mShadowData.pointShadowFar01[i] = pointLights[i]->mShadow->mCamera->mFar;
		}
	}
	for (int i = 4; i < count && i < 8; i++) {
		if (pointLights[i]->mShadow && pointLights[i]->mShadow->mCamera) {
			mShadowData.pointShadowFar23[i - 4] = pointLights[i]->mShadow->mCamera->mFar;
		}
	}

	glBindBuffer(GL_UNIFORM_BUFFER, mShadowUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(ShadowUBOData), &mShadowData);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_SHADOW, mShadowUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UBOManager::updateRenderSettings(
	Camera* camera,
	RenderMode mode,
	int shadowType,
	glm::vec3 ambientColor
) {
	mSettingsData.ambientColor = glm::vec4(ambientColor, 0.0f);
	mSettingsData.cameraPosition = glm::vec4(camera->mPosition, 0.0f);
	mSettingsData.renderParams = glm::vec4(1.0f, static_cast<float>(static_cast<int>(mode)), static_cast<float>(shadowType), 0.0f);

	glBindBuffer(GL_UNIFORM_BUFFER, mSettingsUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(RenderSettingsUBOData), &mSettingsData);
	glBindBufferBase(GL_UNIFORM_BUFFER, BINDING_RENDER_SETTINGS, mSettingsUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void UBOManager::bindToShader(GLuint program) {
	GLuint blockIdx;

	blockIdx = glGetUniformBlockIndex(program, "LightBlock");
	if (blockIdx != GL_INVALID_INDEX) {
		glUniformBlockBinding(program, blockIdx, BINDING_LIGHTS);
	}

	blockIdx = glGetUniformBlockIndex(program, "ShadowBlock");
	if (blockIdx != GL_INVALID_INDEX) {
		glUniformBlockBinding(program, blockIdx, BINDING_SHADOW);
	}

	blockIdx = glGetUniformBlockIndex(program, "RenderSettingsBlock");
	if (blockIdx != GL_INVALID_INDEX) {
		glUniformBlockBinding(program, blockIdx, BINDING_RENDER_SETTINGS);
	}
}
