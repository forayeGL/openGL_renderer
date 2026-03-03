#pragma once
#include "material.h"
#include "../texture.h"

class ScreenMaterial :public Material {
public:
	ScreenMaterial();
	~ScreenMaterial();

	const char* getVertexShaderPath() const override;
	const char* getFragmentShaderPath() const override;
	void applyUniforms(
		Shader* shader,
		Mesh* mesh,
		Camera* camera,
		const std::vector<PointLight*>& pointLights
	) override;

public:
	Texture* mScreenTexture{ nullptr };

	float	 mExposure{ 1.0f };
};