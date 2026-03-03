#pragma once
#include "material.h"
#include "../texture.h"

class PhongEnvMaterial :public Material {
public:
	PhongEnvMaterial();
	~PhongEnvMaterial();

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
	Texture*	mEnv{ nullptr };
	float		mShiness{ 1.0f };
};