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
#include "../glframework/ibl/IBLGenerator.h"
#include "gui/gui_system.h"
#include <vector>
#include <memory>
#include <string>

class ScreenMaterial;
class DeferredRenderPipeline;

/**
 * @brief 渲染器应用程序
 * 
 * 管理整个渲染应用的生命周期：
 * - 初始化OpenGL上下文和窗口
 * - 创建场景、相机、光源
 * - 选择并驱动渲染管线（前向/延迟）
 * - 管理GUI系统
 * - 处理用户输入
 */
class RendererApp {
public:
	RendererApp(int width, int height);
	~RendererApp();

	bool init();
	void run();
	void shutdown();

	/// 在init()之前注入自定义管线以替换默认管线
	void setPipeline(std::unique_ptr<IRenderPipeline> pipeline);

private:
	void prepareScene();
	void prepareForwardScene();
	void prepareDefferedDemo();
	void prepareForwardDemo();
	void prepareCamera();
	void buildGuiPanels();

	/// 初始化IBL资源（环境贴图、辐照度、预滤波、BRDF LUT）
	void prepareIBL();

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

	/// IBL资源生成器和句柄
	IBLGenerator mIBLGenerator;
	GLuint mEnvCubemap{ 0 };       ///< 环境立方体贴图
	GLuint mIrradianceMap{ 0 };    ///< 辐照度贴图
	GLuint mPrefilteredMap{ 0 };   ///< 预滤波环境贴图
	GLuint mBRDFLUT{ 0 };          ///< BRDF积分LUT
};
