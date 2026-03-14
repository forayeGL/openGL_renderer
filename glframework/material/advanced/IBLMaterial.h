#pragma once
#include "../material.h"
#include "../../texture.h"

class IBLMaterial :public Material {
public:
	IBLMaterial();
	~IBLMaterial();

	const char* getVertexShaderPath() const override;
	const char* getFragmentShaderPath() const override;
	void applyUniforms(
		Shader* shader,
		Mesh* mesh,
		Camera* camera,
		const std::vector<PointLight*>& pointLights
	) override;

public:
	Texture* mAlbedo{ nullptr };
	Texture* mRoughness{ nullptr };
	Texture* mNormal{ nullptr };
	Texture* mMetallic{ nullptr };
	Texture* mIrradianceIndirect{ nullptr };
};