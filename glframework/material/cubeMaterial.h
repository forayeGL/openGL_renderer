#pragma once
#include "material.h"
#include "../texture.h"

class CubeMaterial :public Material {
public:
	CubeMaterial();
	~CubeMaterial();

	const char* getVertexShaderPath() const override;
	const char* getFragmentShaderPath() const override;
	void applyUniforms(
		Shader* shader,
		Mesh* mesh,
		Camera* camera,
		const std::vector<PointLight*>& pointLights
	) override;

public:
	Texture* mDiffuse{ nullptr };
};