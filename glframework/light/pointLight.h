#pragma once
#include "light.h"
#include "../object.h"

class PointLight :public Light{
public:
	PointLight();
	~PointLight();

public:
	float mK2 = 1.0f;
	float mK1 = 1.0f;
	float mKc = 1.0f;

	// <= 0 表示自动根据衰减参数和亮度估算有效半径
	float mRange = -1.0f;
};