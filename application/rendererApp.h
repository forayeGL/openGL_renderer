#pragma once

#include "../glframework/core.h"
#include "camera/camera.h"
#include "camera/GameCameraControl.h"
#include "../glframework/scene.h"
#include "../glframework/light/directionalLight.h"
#include "../glframework/light/pointLight.h"
#include "../glframework/mesh/mesh.h"
#include "../glframework/renderer/i_render_pipeline.h"
#include "../glframework/renderer/render_context.h"
#include "gui/gui_system.h"
#include <vector>
#include <memory>
#include <string>

class ScreenMaterial;

class RendererApp {
public:
	RendererApp(int width, int height);
	~RendererApp();

	bool init();
	void run();
	void shutdown();

	// Inject a custom pipeline before init() to replace the default one.
	void setPipeline(std::unique_ptr<IRenderPipeline> pipeline);

private:
	void prepareScene();
	void prepareCamera();
	void buildGuiPanels();

	void addObject(const std::string& type);
	void removeSelectedObject();
	void addPointLight();
	void removePointLight(int idx);

	static void onResize(int width, int height);
	static void onKey(int key, int action, int mods);
	static void onMouse(int button, int action, int mods);
	static void onCursor(double xpos, double ypos);
	static void onScroll(double offset);

private:
	static RendererApp* sInstance;

	int mWidth{ 0 };
	int mHeight{ 0 };

	std::unique_ptr<IRenderPipeline>   mPipeline;
	std::unique_ptr<Scene>             mMainScene;
	std::unique_ptr<Scene>             mPostScene;
	ScreenMaterial*                    mScreenMat{ nullptr };

	std::unique_ptr<Camera>            mCamera;
	std::unique_ptr<GameCameraControl> mCameraControl;

	DirectionalLight*        mDirLight{ nullptr };
	std::vector<PointLight*> mPointLights;
	std::vector<Mesh*>       mDynamicObjects;
	int                      mSelectedObject{ -1 };

	RenderContext mRenderCtx;
	GuiSystem     mGuiSystem;
};
