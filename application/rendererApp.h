#pragma once

#include "../glframework/core.h"
#include "camera/camera.h"
#include "camera/GameCameraControl.h"
#include "../glframework/scene.h"
#include "../glframework/light/pointLight.h"
#include <vector>
#include <memory>

class RenderPipeline;
class ScreenMaterial;

class RendererApp {
public:
	RendererApp(int width, int height);
	~RendererApp();

	bool init();
	void run();
	void shutdown();

private:
	void prepareScene();
	void prepareCamera();
	void initImGui();
	void renderImGui();

	static void onResize(int width, int height);
	static void onKey(int key, int action, int mods);
	static void onMouse(int button, int action, int mods);
	static void onCursor(double xpos, double ypos);
	static void onScroll(double offset);

private:
	static RendererApp* sInstance;

	int mWidth{ 0 };
	int mHeight{ 0 };

	std::unique_ptr<RenderPipeline>    mPipeline;
	std::unique_ptr<Scene>             mMainScene;
	std::unique_ptr<Scene>             mPostScene;

	ScreenMaterial*    mScreenMat{ nullptr }; // non-owning, owned by mPostScene tree

	std::unique_ptr<Camera>            mCamera;
	std::unique_ptr<GameCameraControl> mCameraControl;

	std::vector<PointLight*> mPointLights;
	glm::vec3 mClearColor{};
};
