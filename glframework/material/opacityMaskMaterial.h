#pragma once
#include "material.h"
#include "../texture.h"

class OpacityMaskMaterial :public Material {
public:
	OpacityMaskMaterial();
	~OpacityMaskMaterial();

	const char* getVertexShaderPath() const override;
	const char* getFragmentShaderPath() const override;
	void applyUniforms(
		Shader* shader,
		Mesh* mesh,
		Camera* camera,
		const std::vector<PointLight*>& pointLights
	) override;

public:
	Texture*	mDiffuse{ nullptr };
	Texture*	mOpacityMask{ nullptr };
	float		mShiness{ 1.0f };
};