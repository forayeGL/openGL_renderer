#pragma once
#include "material.h"
#include "../texture.h"

class GrassInstanceMaterial :public Material {
public:
	GrassInstanceMaterial();
	~GrassInstanceMaterial();

	const char* getVertexShaderPath() const override;
	const char* getFragmentShaderPath() const override;
	void applyUniforms(
		Shader* shader,
		Mesh* mesh,
		Camera* camera,
		const std::vector<PointLight*>& pointLights
	) override;

public:
    // Base map aliases kept for compatibility.
	Texture* mDiffuse{ nullptr };      // == mAlbedo
	Texture* mOpacityMask{ nullptr };  // == mOpacity

	// PBR texture slots.
	Texture* mAlbedo{ nullptr };
	Texture* mNormal{ nullptr };
	Texture* mRoughness{ nullptr };
	Texture* mMetallic{ nullptr };
	Texture* mAO{ nullptr };
	Texture* mOpacity{ nullptr };
	Texture* mThickness{ nullptr };

	// IBL resources.
	GLuint mIrradianceIndirect{ 0 };
	GLuint mPrefilteredMap{ 0 };
	GLuint mBRDFLUT{ 0 };

	// Texture usage switches.
	bool mUseAlbedoMap{ true };
	bool mUseNormalMap{ false };
	bool mUseRoughnessMap{ false };
	bool mUseMetallicMap{ false };
	bool mUseAOMap{ false };
	bool mUseOpacityMap{ true };
	bool mUseThicknessMap{ false };
	bool mUseIBL{ true };
	bool mUseTransmission{ true };

	// Scalar/vector fallback parameters.
	glm::vec3 mAlbedoValue{ 0.35f, 0.55f, 0.25f };
	float mRoughnessValue{ 0.85f };
	float mMetallicValue{ 0.0f };
	float mAOValue{ 1.0f };
	float mOpacityValue{ 1.0f };
	float mThicknessValue{ 0.6f };
	float mSpecularBoost{ 0.2f };
	glm::vec3 mSubsurfaceColor{ 0.45f, 0.9f, 0.35f };
	float mTransmissionStrength{ 0.35f };
	float mAlphaCutoff{ 0.35f };

	// World-space UV controls.
	float mUVScale{ 1.0f };
	float mBrightness{ 1.0f };

	// Wind controls.
	glm::vec3 mWindDirection{ 1.0f, 0.0f, 0.2f };
	float mWindAmplitude{ 0.08f };
	float mWindFrequency{ 1.2f };
	float mWindGustAmplitude{ 0.03f };
	float mWindGustFrequency{ 3.2f };
	float mPhaseScale{ 2.0f };
};