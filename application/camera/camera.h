#pragma once

#include "../../glframework/core.h"

class Camera {
public:
	Camera();
	virtual ~Camera();

	glm::mat4 getViewMatrix();
	virtual glm::mat4 getProjectionMatrix();
	virtual void scale(float deltaScale);

	void setProjectionJitter(const glm::vec2& jitterNdc) { mProjectionJitter = jitterNdc; }
	void clearProjectionJitter() { mProjectionJitter = glm::vec2(0.0f); }
	const glm::vec2& getProjectionJitter() const { return mProjectionJitter; }

protected:
	glm::mat4 applyProjectionJitter(const glm::mat4& proj) const;

public:
	glm::vec3 mPosition{ 0.0f,0.0f,5.0f };
	glm::vec3 mUp{ 0.0f, 1.0f, 0.0f };
	glm::vec3 mRight{ 1.0f,0.0f,0.0f };

	float mNear = 0.0f;
	float mFar = 0.0f;

private:
	glm::vec2 mProjectionJitter{ 0.0f, 0.0f };
};