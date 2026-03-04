#pragma once
#include "material.h"

class WhiteMaterial :public Material {
public:
	WhiteMaterial();
	~WhiteMaterial();

	const char* getVertexShaderPath() const override;
	const char* getFragmentShaderPath() const override;
	void applyUniforms(
		Shader* shader,
		Mesh* mesh,
		Camera* camera,
		const std::vector<PointLight*>& pointLights
	) override;
};