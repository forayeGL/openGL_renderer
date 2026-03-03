#include "rendererApp.h"
#include "Application.h"

#include "../glframework/texture.h"
#include "../glframework/geometry.h"
#include "../glframework/mesh/mesh.h"

#include "../glframework/material/screenMaterial.h"
#include "../glframework/material/cubeMaterial.h"
#include "../glframework/material/advanced/pbrMaterial.h"

#include "../glframework/renderer/render_pipeline.h"
#include "../glframework/renderer/bloom.h"

#include "camera/perspectiveCamera.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

#include "../wrapper/checkError.h"

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
	initImGui();

	return true;
}

void RendererApp::run() {
	while (glApp->update()) {
		mCameraControl->update();

		mPipeline->setClearColor(mClearColor);
		mPipeline->execute(mMainScene.get(), mPostScene.get(), mCamera.get(), mPointLights);

		renderImGui();
	}
}

void RendererApp::shutdown() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	mPipeline.reset();
	mMainScene.reset();
	mPostScene.reset();
	mCamera.reset();
	mCameraControl.reset();

	for (auto* pl : mPointLights) {
		delete pl;
	}
	mPointLights.clear();

	glApp->destroy();
}

void RendererApp::prepareScene() {
	mPipeline = std::make_unique<RenderPipeline>(mWidth, mHeight);
	mMainScene = std::make_unique<Scene>();
	mPostScene = std::make_unique<Scene>();

	//pass 01
	auto geometry = Geometry::createSphere(1.0f);
	auto pbrMat = new PbrMaterial();
	pbrMat->mAlbedo = new Texture("assets/textures/pbr/slab_tiles_diff_2k.jpg", 0, GL_SRGB_ALPHA);
	pbrMat->mNormal = Texture::createNearestTexture("assets/textures/pbr/slab_tiles_nor_gl_2k.jpg");
	pbrMat->mRoughness = Texture::createNearestTexture("assets/textures/pbr/slab_tiles_rough_2k.jpg");
	pbrMat->mMetallic = Texture::createNearestTexture("assets/textures/pbr/slab_tiles_arm_2k.jpg");
	pbrMat->mIrradianceIndirect = Texture::createExrCubeMap(
		{
			"assets/textures/pbr/IBL/env_0.exr",
			"assets/textures/pbr/IBL/env_1.exr",
			"assets/textures/pbr/IBL/env_2.exr",
			"assets/textures/pbr/IBL/env_3.exr",
			"assets/textures/pbr/IBL/env_4.exr",
			"assets/textures/pbr/IBL/env_5.exr",
		}
	);

	for (int i = 0; i < 5; i++) {
		for (int j = 0; j < 5; j++) {
			auto mesh = new Mesh(geometry, pbrMat);
			mesh->setPosition(glm::vec3(i * 2.5, j * 2.5, 0.0f));
			mMainScene->addChild(mesh);
		}
	}

	auto boxGeo = Geometry::createBox(1.0f);
	auto boxMat = new CubeMaterial();
	boxMat->mDiffuse = Texture::createExrTexture("assets/textures/pbr/qwantani_dusk_1_4k.exr");
	auto boxMesh = new Mesh(boxGeo, boxMat);
	mMainScene->addChild(boxMesh);

	//pass 02 postProcessPass
	auto sgeo = Geometry::createScreenPlane();
	mScreenMat = new ScreenMaterial();
	mScreenMat->mScreenTexture = mPipeline->getResolveColorAttachment();
	auto smesh = new Mesh(sgeo, mScreenMat);
	mPostScene->addChild(smesh);

	glm::vec3 lightPositions[] = {
		glm::vec3(-3.0f,  3.0f, 10.0f),
		glm::vec3( 3.0f,  3.0f, 10.0f),
		glm::vec3(-3.0f, -3.0f, 10.0f),
		glm::vec3( 3.0f, -3.0f, 10.0f),
	};
	glm::vec3 lightColors[] = {
		glm::vec3(300.0f, 300.0f, 300.0f),
		glm::vec3(300.0f, 300.0f, 300.0f),
		glm::vec3(300.0f, 300.0f, 300.0f),
		glm::vec3(300.0f, 300.0f, 300.0f)
	};
	for (int i = 0; i < 4; i++) {
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

void RendererApp::initImGui() {
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(glApp->getWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void RendererApp::renderImGui() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("MaterialEditor");
	ImGui::SliderFloat("exposure:", &mScreenMat->mExposure, 0.0f, 10.0f);
	auto* bloom = mPipeline->getBloom();
	ImGui::SliderFloat("Threshold:", &bloom->mThreshold, 0.0f, 100.0f);
	ImGui::SliderFloat("BloomAttenuation:", &bloom->mBloomAttenuation, 0.0f, 1.0f);
	ImGui::SliderFloat("BloomIntensity:", &bloom->mBloomIntensity, 0.0f, 1.0f);
	ImGui::SliderFloat("BloomRadius:", &bloom->mBloomRadius, 0.0f, 1.0f);
	ImGui::End();

	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(glApp->getWindow(), &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
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
