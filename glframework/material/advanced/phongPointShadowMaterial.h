#pragma once
#include "../material.h"
#include "../../texture.h"

class PhongPointShadowMaterial :public Material {
public:
	PhongPointShadowMaterial();
	~PhongPointShadowMaterial();

public:
	Texture*	mDiffuse{ nullptr };
	Texture*	mSpecularMask{ nullptr };
	float		mShiness{ 1.0f };
};