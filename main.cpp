#include <iostream>

#include "glframework/core.h"
#include "glframework/shader.h"
#include <string>
#include <assert.h>//断言
#include "wrapper/checkError.h"
#include "application/Application.h"
#include "glframework/texture.h"

//引入相机+控制器
#include "application/camera/perspectiveCamera.h"
#include "application/camera/orthographicCamera.h"
#include "application/camera/trackBallCameraControl.h"
#include "application/camera/GameCameraControl.h"

#include "glframework/geometry.h"
#include "glframework/material/phongMaterial.h"
#include "glframework/material/whiteMaterial.h"
#include "glframework/material/depthMaterial.h"
#include "glframework/material/opacityMaskMaterial.h"
#include "glframework/material/screenMaterial.h"
#include "glframework/material/cubeMaterial.h"
#include "glframework/material/phongEnvMaterial.h"
#include "glframework/material/phongInstanceMaterial.h"
#include "glframework/material/grassInstanceMaterial.h"
#include "glframework/material/advanced/phongNormalMaterial.h"
#include "glframework/material/advanced/phongParallaxMaterial.h"
#include "glframework/material/advanced/phongShadowMaterial.h"
#include "glframework/material/advanced/phongCSMShadowMaterial.h"
#include "glframework/material/advanced/phongPointShadowMaterial.h"
#include "glframework/material/advanced/pbrMaterial.h"


#include "glframework/mesh/mesh.h"
#include "glframework/mesh/instancedMesh.h"
#include "glframework/renderer/renderer.h"
#include "glframework/light/pointLight.h"
#include "glframework/light/spotLight.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "glframework/scene.h"
#include "application/assimpLoader.h"

#include "glframework/framebuffer/framebuffer.h"

#include "application/assimpInstanceLoader.h"

#include "glframework/renderer/bloom.h"	 

Renderer* renderer = nullptr;
Bloom* bloom = nullptr;

Scene* sceneOff = nullptr;
Scene* scene = nullptr;
Framebuffer* fboMulti = nullptr;
Framebuffer* fboResolve = nullptr;

ScreenMaterial* screenMat = nullptr;
PbrMaterial* material = nullptr;

Mesh* upPlane = nullptr;

int WIDTH = 2560;
int HEIGHT = 1440;


std::vector<PointLight*> pointLights{};


Camera* camera = nullptr;
GameCameraControl* cameraControl = nullptr;

glm::vec3 clearColor{};

void OnResize(int width, int height) {
	GL_CALL(glViewport(0, 0, width, height));
}

void OnKey(int key, int action, int mods) {
	cameraControl->onKey(key, action, mods);
}

//鼠标按下/抬起
void OnMouse(int button, int action, int mods) {
	double x, y;
	glApp->getCursorPosition(&x, &y);
	cameraControl->onMouse(button, action, x, y);
}

//鼠标移动
void OnCursor(double xpos, double ypos) {
	cameraControl->onCursor(xpos, ypos);
}

//鼠标滚轮
void OnScroll(double offset) {
	cameraControl->onScroll(offset);
}


void prepare() {
	fboMulti = Framebuffer::createMultiSampleHDRFbo(WIDTH, HEIGHT);
	fboResolve = Framebuffer::createHDRFbo(WIDTH, HEIGHT);

	renderer = new Renderer();
	bloom = new Bloom(WIDTH, HEIGHT);
	sceneOff = new Scene();
	scene = new Scene();

	//pass 01
	auto geometry = Geometry::createSphere(1.0f);
	material = new PbrMaterial();
	material->mAlbedo = new Texture("assets/textures/pbr/slab_tiles_diff_2k.jpg", 0, GL_SRGB_ALPHA);
	material->mNormal = Texture::createNearestTexture("assets/textures/pbr/slab_tiles_nor_gl_2k.jpg");
	material->mRoughness = Texture::createNearestTexture("assets/textures/pbr/slab_tiles_rough_2k.jpg");
	material->mMetallic = Texture::createNearestTexture("assets/textures/pbr/slab_tiles_arm_2k.jpg");
	material->mIrradianceIndirect = Texture::createExrCubeMap(
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
			auto mesh = new Mesh(geometry, material);
			mesh->setPosition(glm::vec3(i * 2.5, j * 2.5, 0.0f));
			sceneOff->addChild(mesh);
		}
	}

	auto boxGeo = Geometry::createBox(1.0f);
	auto boxMat = new CubeMaterial();
	boxMat->mDiffuse = Texture::createExrTexture("assets/textures/pbr/qwantani_dusk_1_4k.exr");
	auto boxMesh = new Mesh(boxGeo, boxMat);
	sceneOff->addChild(boxMesh);

	//pass 02 postProcessPass:后处理pass
	auto sgeo = Geometry::createScreenPlane();
	screenMat = new ScreenMaterial();
	screenMat->mScreenTexture = fboResolve->mColorAttachment;
	auto smesh = new Mesh(sgeo, screenMat);
	scene->addChild(smesh);

	
	glm::vec3 lightPositions[] = {
			glm::vec3(-3.0f,  3.0f, 10.0f),
			glm::vec3(3.0f,  3.0f, 10.0f),
			glm::vec3(-3.0f, -3.0f, 10.0f),
			glm::vec3(3.0f, -3.0f, 10.0f),
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
		pointLights.push_back(pointLight);
	}

}


void prepareCamera() {
	float size = 10.0f;
	//camera = new OrthographicCamera(-size, size, size, -size, size, -size);
	camera = new PerspectiveCamera(
		60.0f, 
		(float)glApp->getWidth() / (float)glApp->getHeight(),
		0.1f,
		1000.0f
	);

	cameraControl = new GameCameraControl();
	cameraControl->setCamera(camera);
	cameraControl->setSensitivity(0.4f);
	cameraControl->setSpeed(0.1f);
}



void initIMGUI() {
	ImGui::CreateContext();//创建imgui上下文
	ImGui::StyleColorsDark(); // 选择一个主题

	// 设置ImGui与GLFW和OpenGL的绑定
	ImGui_ImplGlfw_InitForOpenGL(glApp->getWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 460");
}

void renderIMGUI() {
	//1 开启当前的IMGUI渲染
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//2 决定当前的GUI上面有哪些控件，从上到下
	ImGui::Begin("MaterialEditor");
	ImGui::SliderFloat("exposure:", &screenMat->mExposure, 0.0f, 10.0f);
	ImGui::SliderFloat("Threshold:", &bloom->mThreshold, 0.0f, 100.0f);
	ImGui::SliderFloat("BloomAttenuation:", &bloom->mBloomAttenuation, 0.0f, 1.0f);
	ImGui::SliderFloat("BloomIntensity:", &bloom->mBloomIntensity, 0.0f, 1.0f);
	ImGui::SliderFloat("BloomRadius:", &bloom->mBloomRadius, 0.0f, 1.0f);

	ImGui::End();

	//3 执行UI渲染
	ImGui::Render();
	//获取当前窗体的宽高
	int display_w, display_h;
	glfwGetFramebufferSize(glApp->getWindow(), &display_w, &display_h);
	//重置视口大小
	glViewport(0, 0, display_w, display_h);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}


int main() {
	if (!glApp->init(WIDTH, HEIGHT)) {
		return -1;
	}

	glApp->setResizeCallback(OnResize);
	glApp->setKeyBoardCallback(OnKey);
	glApp->setMouseCallback(OnMouse);
	glApp->setCursorCallback(OnCursor);
	glApp->setScrollCallback(OnScroll);

	//设置opengl视口以及清理颜色
	GL_CALL(glViewport(0, 0, WIDTH, HEIGHT));
	GL_CALL(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));

	prepareCamera();

	prepare();
	
	initIMGUI();

	while (glApp->update()) {
		cameraControl->update();

		renderer->setClearColor(clearColor);
		renderer->render(sceneOff, camera, pointLights,fboMulti->mFBO);
		renderer->msaaResolve(fboMulti, fboResolve);
		bloom->doBloom(fboResolve);
		renderer->render(scene, camera, pointLights);

		renderIMGUI();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	delete renderer;
	delete bloom;
	delete sceneOff;
	delete scene;
	delete fboMulti;
	delete fboResolve;
	delete camera;
	delete cameraControl;

	for (auto* pl : pointLights) {
		delete pl;
	}
	pointLights.clear();

	glApp->destroy();

	return 0;
}