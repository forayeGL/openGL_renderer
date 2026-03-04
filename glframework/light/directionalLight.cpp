#include "directionalLight.h"
#include "shadow/directionalLightShadow.h"
#include "shadow/directionalLightCSMShadow.h"

/// <summary>
/// 可以更改为CSMShadow以启用级联阴影
/// </summary>
DirectionalLight::DirectionalLight() {
	mShadow = new DirectionalLightShadow();
}

DirectionalLight::~DirectionalLight() {

}