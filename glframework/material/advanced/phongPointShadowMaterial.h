#pragma once
#include "../material.h"
#include "../../texture.h"

class PhongPointShadowMaterial :public Material {
public:
	PhongPointShadowMaterial();
	~PhongPointShadowMaterial();

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
	float		mShiness{ 1.0f };
};