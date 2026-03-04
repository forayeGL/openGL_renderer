#pragma once

#include "cameraControl.h"

class GameCameraControl :public CameraControl {
public:
	GameCameraControl();
	~GameCameraControl();

	void onCursor(double xpos, double ypos)override; 
	void update()override;

	void setSpeed(float s) { mSpeed = s; }
	float getSpeed() const { return mSpeed; }

private:
	void pitch(float angle);
	void yaw(float angle);

private:
	float mPitch{ 0.0f };
	float mSpeed{ 0.1f };
};