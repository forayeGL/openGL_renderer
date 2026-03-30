#include "camera.h"

//可以在这里把初始化参数都传进来
Camera::Camera() {

}

Camera::~Camera() {

}

glm::mat4 Camera::getViewMatrix() {
	//lookat 
	//-eye:相机位置（使用mPosition）
	//-center：看向世界坐标的哪个点
	//-top：穹顶（使用mUp替代）
	glm::vec3 front = glm::cross(mUp, mRight);
	glm::vec3 center = mPosition + front;

	return glm::lookAt(mPosition, center, mUp);
}

glm::mat4 Camera::getProjectionMatrix() {
	return glm::identity<glm::mat4>();
}

void Camera::scale(float deltaScale) {

}

glm::mat4 Camera::applyProjectionJitter(const glm::mat4& proj) const {
	glm::mat4 jittered = proj;
	// OpenGL clip-space jitter: translate projection center in NDC.
	jittered[2][0] += mProjectionJitter.x;
	jittered[2][1] += mProjectionJitter.y;
	return jittered;
}
