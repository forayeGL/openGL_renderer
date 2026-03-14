#include "rendererApp.h"
#include "Application.h"

#include "../glframework/texture.h"
#include "../glframework/geometry.h"
#include "../glframework/mesh/mesh.h"

#include "../glframework/material/screenMaterial.h"
#include "../glframework/material/cubeMaterial.h"
#include "../glframework/material/phongMaterial.h"
#include "../glframework/material/advanced/pbrMaterial.h"
#include "../glframework/material/advanced/phongShadowMaterial.h"
#include "../glframework/material/advanced/phongPointShadowMaterial.h"

#include "../glframework/renderer/default_render_pipeline.h"
#include "../glframework/renderer/bloom.h"

#include "camera/perspectiveCamera.h"
#include "gui/scene_panel.h"
#include "gui/lighting_panel.h"
#include "gui/rendering_panel.h"

#include "../wrapper/checkError.h"

#include <algorithm>

RendererApp* RendererApp::sInstance = nullptr;

RendererApp::RendererApp(int width, int height)
	: mWidth(width), mHeight(height) {
	sInstance = this;
}

RendererApp::~RendererApp() {
	if (sInstance == this) {
		sInstance = nullptr;
	}
}

void RendererApp::setPipeline(std::unique_ptr<IRenderPipeline> pipeline) {
	mPipeline = std::move(pipeline);
}

bool RendererApp::init() {
	if (!glApp->init(mWidth, mHeight)) {
		return false;
	}

	glApp->setResizeCallback(onResize);
	glApp->setKeyBoardCallback(onKey);
	glApp->setMouseCallback(onMouse);
	glApp->setCursorCallback(onCursor);
	glApp->setScrollCallback(onScroll);

	GL_CALL(glViewport(0, 0, mWidth, mHeight));
	GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));

	prepareCamera();
	prepareScene();
	buildGuiPanels();
	mGuiSystem.init(glApp->getWindow());

	return true;
}

void RendererApp::run() {
	while (glApp->update()) {
		mCameraControl->update();

		// Fill RenderContext from current app state
		mRenderCtx.mainScene   = mMainScene.get();
		mRenderCtx.postScene   = mPostScene.get();
		mRenderCtx.camera      = mCamera.get();
		mRenderCtx.dirLight    = mDirLight;
		mRenderCtx.pointLights = &mPointLights;

		mPipeline->execute(mRenderCtx);
		mGuiSystem.render(glApp->getWindow());
	}
}

void RendererApp::shutdown() {
	mGuiSystem.shutdown();

	mPipeline.reset();
	mMainScene.reset();
	mPostScene.reset();
	mCamera.reset();
	mCameraControl.reset();

	delete mDirLight;
	mDirLight = nullptr;

	for (auto* pl : mPointLights) {
		delete pl;
	}
	mPointLights.clear();
	mDynamicObjects.clear();

	glApp->destroy();
}

void RendererApp::prepareScene() {
	// Use injected pipeline if set, otherwise create the default one.
	if (!mPipeline) {
		auto defaultPipeline = std::make_unique<DefaultRenderPipeline>();
		defaultPipeline->init(mWidth, mHeight);
		mPipeline = std::move(defaultPipeline);
	}
	mMainScene = std::make_unique<Scene>();
	mPostScene = std::make_unique<Scene>();

	// Ground plane
	auto planeGeo = Geometry::createPlane(50.0f, 50.0f);
	auto planeMat = new PhongShadowMaterial();
	planeMat->mDiffuse = Texture::createTexture("assets/textures/pbr/slab_tiles_diff_2k.jpg", 0);
	planeMat->mShiness = 32.0f;
	auto planeMesh = new Mesh(planeGeo, planeMat);
	planeMesh->setAngleX(-90.0f);
	planeMesh->setPosition(glm::vec3(0.0f, -2.0f, 0.0f));
	mMainScene->addChild(planeMesh);

	// Initial sphere
	auto sphereGeo = Geometry::createSphere(1.0f);
	auto sphereMat = new PhongShadowMaterial();
	sphereMat->mDiffuse = Texture::createTexture("assets/textures/pbr/slab_tiles_diff_2k.jpg", 0);
	sphereMat->mShiness = 64.0f;
	auto sphereMesh = new Mesh(sphereGeo, sphereMat);
	sphereMesh->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
	mMainScene->addChild(sphereMesh);
	mDynamicObjects.push_back(sphereMesh);

	// Initial cube
	auto boxGeo = Geometry::createBox(1.5f);
	auto boxMat = new PhongShadowMaterial();
	boxMat->mDiffuse = Texture::createTexture("assets/textures/pbr/slab_tiles_diff_2k.jpg", 0);
	boxMat->mShiness = 64.0f;
	auto boxMesh = new Mesh(boxGeo, boxMat);
	boxMesh->setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
	mMainScene->addChild(boxMesh);
	mDynamicObjects.push_back(boxMesh);

	// Skybox
	auto skyGeo = Geometry::createBox(1.0f);
	auto skyMat = new CubeMaterial();
	skyMat->mDiffuse = Texture::createExrTexture("assets/textures/pbr/qwantani_dusk_1_4k.exr");
	auto skyMesh = new Mesh(skyGeo, skyMat);
	mMainScene->addChild(skyMesh);

	// Post-processing pass
	auto sgeo = Geometry::createScreenPlane();
	mScreenMat = new ScreenMaterial();
	mScreenMat->mScreenTexture = mPipeline->getResolveColorAttachment();
	auto smesh = new Mesh(sgeo, mScreenMat);
	mPostScene->addChild(smesh);

	// Populate RenderContext fields that don't change between frames
	mRenderCtx.clearColor    = glm::vec3(0.0f);
	mRenderCtx.ambientColor  = glm::vec3(0.15f);
	mRenderCtx.renderModeIdx = 0;

	// Directional light
	mDirLight = new DirectionalLight();
	mDirLight->setPosition(glm::vec3(10.0f, 20.0f, 10.0f));
	mDirLight->setAngleX(-60.0f);
	mDirLight->setAngleY(30.0f);
	mDirLight->mColor = glm::vec3(1.0f);
	mDirLight->mIntensity = 1.5f;
	mDirLight->mSpecularIntensity = 0.5f;

	// Point lights
	glm::vec3 lightPositions[] = {
		glm::vec3(-5.0f,  5.0f, 5.0f),
		glm::vec3( 5.0f,  5.0f, 5.0f),
	};
	glm::vec3 lightColors[] = {
		glm::vec3(50.0f, 50.0f, 50.0f),
		glm::vec3(50.0f, 50.0f, 50.0f),
	};
	for (int i = 0; i < 2; i++) {
		auto pointLight = new PointLight();
		pointLight->setPosition(lightPositions[i]);
		pointLight->mColor = lightColors[i];
		mPointLights.push_back(pointLight);
	}
}

void RendererApp::prepareCamera() {
	mCamera = std::make_unique<PerspectiveCamera>(
		60.0f,
		(float)glApp->getWidth() / (float)glApp->getHeight(),
		0.1f,
		1000.0f
	);

	mCameraControl = std::make_unique<GameCameraControl>();
	mCameraControl->setCamera(mCamera.get());
	mCameraControl->setSensitivity(0.4f);
	mCameraControl->setSpeed(0.1f);
}

void RendererApp::buildGuiPanels() {
	// Cast to DefaultRenderPipeline to access bloom / renderer.
	// If a custom pipeline is used it should either expose those accessors
	// or register its own panels before init() is called.
	auto* dp = dynamic_cast<DefaultRenderPipeline*>(mPipeline.get());

	mGuiSystem.addPanel(std::make_unique<ScenePanel>(
		mDynamicObjects,
		mSelectedObject,
		[this](const std::string& t) { addObject(t); },
		[this]() { removeSelectedObject(); }
	));

	mGuiSystem.addPanel(std::make_unique<LightingPanel>(
		mDirLight,
		mPointLights,
		[this]() { addPointLight(); },
		[this](int i) { removePointLight(i); }
	));

	if (dp) {
		mGuiSystem.addPanel(std::make_unique<RenderingPanel>(
			mRenderCtx.renderModeIdx,
			mRenderCtx.clearColor,
			mRenderCtx.ambientColor,
			mScreenMat,
			dp->getBloom(),
			mCameraControl.get(),
			mRenderCtx.showDebugAxis
		));
	}
}

// renderImGui() has been removed; GuiSystem::render() takes its place.

void RendererApp::addObject(const std::string& type) {
	Mesh* mesh = nullptr;
	auto mat = new PhongShadowMaterial();
	mat->mDiffuse = Texture::createTexture("assets/textures/pbr/slab_tiles_diff_2k.jpg", 0);
	mat->mShiness = 64.0f;

	if (type == "cube") {
		mesh = new Mesh(Geometry::createBox(1.0f), mat);
	}
	else {
		mesh = new Mesh(Geometry::createSphere(0.8f), mat);
	}

	float x = (float)(rand() % 20 - 10);
	float z = (float)(rand() % 20 - 10);
	mesh->setPosition(glm::vec3(x, 0.0f, z));

	mMainScene->addChild(mesh);
	mDynamicObjects.push_back(mesh);
	mSelectedObject = (int)mDynamicObjects.size() - 1;
}

void RendererApp::removeSelectedObject() {
	if (mSelectedObject < 0 || mSelectedObject >= (int)mDynamicObjects.size()) return;

	auto* mesh = mDynamicObjects[mSelectedObject];
	mMainScene->removeChild(mesh);

	delete mesh;
	mDynamicObjects.erase(mDynamicObjects.begin() + mSelectedObject);
	mSelectedObject = -1;
}

void RendererApp::addPointLight() {
	if ((int)mPointLights.size() >= MAX_POINT_LIGHTS) return;

	auto* pl = new PointLight();
	float x = (float)(rand() % 20 - 10);
	float z = (float)(rand() % 20 - 10);
	pl->setPosition(glm::vec3(x, 5.0f, z));
	pl->mColor = glm::vec3(50.0f);
	mPointLights.push_back(pl);
}

void RendererApp::removePointLight(int idx) {
	if (idx < 0 || idx >= (int)mPointLights.size()) return;

	delete mPointLights[idx];
	mPointLights.erase(mPointLights.begin() + idx);
}

void RendererApp::onResize(int width, int height) {
	GL_CALL(glViewport(0, 0, width, height));
}

void RendererApp::onKey(int key, int action, int mods) {
	sInstance->mCameraControl->onKey(key, action, mods);
}

void RendererApp::onMouse(int button, int action, int mods) {
	double x, y;
	glApp->getCursorPosition(&x, &y);
	sInstance->mCameraControl->onMouse(button, action, x, y);
}

void RendererApp::onCursor(double xpos, double ypos) {
	sInstance->mCameraControl->onCursor(xpos, ypos);
}

void RendererApp::onScroll(double offset) {
	sInstance->mCameraControl->onScroll(offset);
}
