#include "orthographicCamera.h"

OrthographicCamera::OrthographicCamera(float l, float r, float b, float t, float n, float f) {
	mL = l;
	mR = r;
	mT = t;
	mB = b;
	mNear = n;
	mFar = f;
}
OrthographicCamera::~OrthographicCamera() {

}

glm::mat4 OrthographicCamera::getProjectionMatrix(){
	float scale = std::pow(2.0f, mScale);
 auto proj = glm::ortho(mL * scale, mR * scale, mB * scale, mT * scale, mNear, mFar);
	return applyProjectionJitter(proj);
}

void OrthographicCamera::scale(float deltaScale) {
	mScale += deltaScale;
}
