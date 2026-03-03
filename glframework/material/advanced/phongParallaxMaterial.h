#pragma once
#include "../material.h"
#include "../../texture.h"

class PhongParallaxMaterial :public Material {
public:
	PhongParallaxMaterial();
	~PhongParallaxMaterial();

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
	Texture*	mSpecularMask{ nullptr };
	Texture*	mNormalMap{ nullptr };
	Texture*	mParallaxMap{ nullptr };

	float		mHeightScale{ 0.1f };
	float		mLayerNum{ 10.0f };

	float		mShiness{ 1.0f };
};