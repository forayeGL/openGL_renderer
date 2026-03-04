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

#include "../glframework/renderer/render_pipeline.h"
#include "../glframework/renderer/bloom.h"

#include "camera/perspectiveCamera.h"

#include "../imgui/imgui.h"
#include "../imgui/imgui_impl_glfw.h"
#include "../imgui/imgui_impl_opengl3.h"

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
		mPipeline->setRenderMode(static_cast<RenderMode>(mRenderModeIdx));
		mPipeline->setAmbientColor(mAmbientColor);
		mPipeline->execute(mMainScene.get(), mPostScene.get(), mCamera.get(), mDirLight, mPointLights);

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
	mPipeline = std::make_unique<RenderPipeline>(mWidth, mHeight);
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

	// ===== Scene Objects =====
	ImGui::Begin("Scene");
	ImGui::Text("Objects: %d", (int)mDynamicObjects.size());
	if (ImGui::Button("Add Cube")) { addObject("cube"); }
	ImGui::SameLine();
	if (ImGui::Button("Add Sphere")) { addObject("sphere"); }

	ImGui::Separator();
	for (int i = 0; i < (int)mDynamicObjects.size(); i++) {
		char label[64];
		snprintf(label, sizeof(label), "Object %d", i);
		if (ImGui::Selectable(label, mSelectedObject == i)) {
			mSelectedObject = i;
		}
	}

	if (mSelectedObject >= 0 && mSelectedObject < (int)mDynamicObjects.size()) {
		ImGui::Separator();
		ImGui::Text("Selected: Object %d", mSelectedObject);
		auto* obj = mDynamicObjects[mSelectedObject];
		glm::vec3 pos = obj->getPosition();
		if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
			obj->setPosition(pos);
		}
		float angleX = 0.0f, angleY = 0.0f, angleZ = 0.0f;
		if (ImGui::DragFloat("Rotate X", &angleX, 1.0f)) { obj->rotateX(angleX); }
		if (ImGui::DragFloat("Rotate Y", &angleY, 1.0f)) { obj->rotateY(angleY); }
		if (ImGui::DragFloat("Rotate Z", &angleZ, 1.0f)) { obj->rotateZ(angleZ); }

		if (ImGui::Button("Remove Selected")) {
			removeSelectedObject();
		}
	}
	ImGui::End();

	// ===== Lighting =====
	ImGui::Begin("Lighting");
	if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
		glm::vec3 dirPos = mDirLight->getPosition();
		if (ImGui::DragFloat3("Dir Position", &dirPos.x, 0.1f)) {
			mDirLight->setPosition(dirPos);
		}
		ImGui::ColorEdit3("Dir Color", &mDirLight->mColor.x);
		ImGui::SliderFloat("Dir Intensity", &mDirLight->mIntensity, 0.0f, 10.0f);
		ImGui::SliderFloat("Dir Specular", &mDirLight->mSpecularIntensity, 0.0f, 5.0f);

		float dirAngleX = 0.0f, dirAngleY = 0.0f;
		if (ImGui::DragFloat("Dir Angle X", &dirAngleX, 0.5f)) { mDirLight->rotateX(dirAngleX); }
		if (ImGui::DragFloat("Dir Angle Y", &dirAngleY, 0.5f)) { mDirLight->rotateY(dirAngleY); }

		if (mDirLight->mShadow) {
			ImGui::SliderFloat("Shadow Bias", &mDirLight->mShadow->mBias, 0.0f, 0.01f, "%.5f");
			ImGui::SliderFloat("PCF Radius", &mDirLight->mShadow->mPcfRadius, 0.0f, 0.1f);
			ImGui::SliderFloat("Disk Tightness", &mDirLight->mShadow->mDiskTightness, 0.0f, 5.0f);
			ImGui::SliderFloat("Light Size", &mDirLight->mShadow->mLightSize, 0.0f, 0.5f);
		}
	}

	ImGui::Separator();
	ImGui::Text("Point Lights: %d / %d", (int)mPointLights.size(), MAX_POINT_LIGHTS);
	if ((int)mPointLights.size() < MAX_POINT_LIGHTS) {
		if (ImGui::Button("Add Point Light")) { addPointLight(); }
	}

	for (int i = 0; i < (int)mPointLights.size(); i++) {
		char header[64];
		snprintf(header, sizeof(header), "Point Light %d", i);
		if (ImGui::CollapsingHeader(header)) {
			std::string id = std::to_string(i);
			glm::vec3 plPos = mPointLights[i]->getPosition();
			if (ImGui::DragFloat3(("PL Pos##" + id).c_str(), &plPos.x, 0.1f)) {
				mPointLights[i]->setPosition(plPos);
			}
			ImGui::ColorEdit3(("PL Color##" + id).c_str(), &mPointLights[i]->mColor.x);
			ImGui::SliderFloat(("PL K2##" + id).c_str(), &mPointLights[i]->mK2, 0.0f, 2.0f);
			ImGui::SliderFloat(("PL K1##" + id).c_str(), &mPointLights[i]->mK1, 0.0f, 2.0f);
			ImGui::SliderFloat(("PL Kc##" + id).c_str(), &mPointLights[i]->mKc, 0.0f, 2.0f);

			if (mPointLights[i]->mShadow) {
				ImGui::SliderFloat(("PL Shadow Bias##" + id).c_str(),
					&mPointLights[i]->mShadow->mBias, 0.0f, 0.01f, "%.5f");
			}

			if (ImGui::Button(("Remove##pl" + id).c_str())) {
				removePointLight(i);
				break;
			}
		}
	}
	ImGui::End();

	// ===== Rendering =====
	ImGui::Begin("Rendering");

	const char* modeNames[] = { "Fill", "Wireframe", "Shadow Only" };
	ImGui::Combo("Render Mode", &mRenderModeIdx, modeNames, 3);

	ImGui::SliderFloat("Exposure", &mScreenMat->mExposure, 0.0f, 10.0f);
	ImGui::ColorEdit3("Clear Color", &mClearColor.x);
	ImGui::ColorEdit3("Ambient Color", &mAmbientColor.x);

	auto* bloom = mPipeline->getBloom();
	if (ImGui::CollapsingHeader("Bloom", ImGuiTreeNodeFlags_DefaultOpen)) {
		ImGui::SliderFloat("Threshold", &bloom->mThreshold, 0.0f, 100.0f);
		ImGui::SliderFloat("Attenuation", &bloom->mBloomAttenuation, 0.0f, 1.0f);
		ImGui::SliderFloat("Intensity", &bloom->mBloomIntensity, 0.0f, 1.0f);
		ImGui::SliderFloat("Radius", &bloom->mBloomRadius, 0.0f, 1.0f);
	}

	if (ImGui::CollapsingHeader("Camera")) {
		float speed = mCameraControl->getSpeed();
		if (ImGui::SliderFloat("Move Speed", &speed, 0.01f, 2.0f)) {
			mCameraControl->setSpeed(speed);
		}
	}
	ImGui::End();

	ImGui::Render();
	int display_w, display_h;
	glfwGetFramebufferSize(glApp->getWindow(), &display_w, &display_h);
	glViewport(0, 0, display_w, display_h);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

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
