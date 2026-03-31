#include "perspectiveCamera.h"

PerspectiveCamera::PerspectiveCamera(float fovy, float aspect, float near, float far) {
	mFovy = fovy;
	mAspect = aspect;
	mNear = near;
	mFar = far;
}
PerspectiveCamera::~PerspectiveCamera() {

}

glm::mat4 PerspectiveCamera::getProjectionMatrix() {
	//注意：传入的是fovy的角度，需要转化为弧度
 auto proj = glm::perspective(glm::radians(mFovy), mAspect, mNear, mFar);
	return applyProjectionJitter(proj);
}

void PerspectiveCamera::scale(float deltaScale) {
	auto front = glm::cross(mUp, mRight);
	mPosition += (front * deltaScale);
}