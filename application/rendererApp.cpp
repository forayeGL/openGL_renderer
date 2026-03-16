#include "rendererApp.h"
#include "Application.h"
#include "assimpInstanceLoader.h"

#include "../glframework/texture.h"
#include "../glframework/geometry.h"
#include "../glframework/mesh/mesh.h"

#include "../glframework/material/screenMaterial.h"
#include "../glframework/material/cubeMaterial.h"
#include "../glframework/material/phongMaterial.h"
#include "../glframework/material/advanced/pbrMaterial.h"
#include "../glframework/material/advanced/phongShadowMaterial.h"
#include "../glframework/material/advanced/phongPointShadowMaterial.h"
#include "../glframework/material/grassInstanceMaterial.h"

#include "../glframework/mesh/instancedMesh.h"

#include "../glframework/renderer/default_render_pipeline.h"
#include "../glframework/renderer/deferred_render_pipeline.h"
#include "../glframework/renderer/bloom.h"

#include "camera/perspectiveCamera.h"
#include "gui/scene_panel.h"
#include "gui/lighting_panel.h"
#include "gui/rendering_panel.h"

#include "../wrapper/checkError.h"

#include <algorithm>

namespace {
	template<typename T>
	T* trackMaterial(std::vector<std::shared_ptr<Material>>& pool, T* mat) {
		if(mat) pool.push_back(std::shared_ptr<Material>(mat));
		return mat;
	}

	template<typename T>
	T* trackGeometry(std::vector<std::shared_ptr<Geometry>>& pool, T* geo) {
		if(geo) pool.push_back(std::shared_ptr<Geometry>(geo));
		return geo;
	}
}

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
	//prepareDefferedDemo();
	//prepareForwardScene();
	//prepareForwardDemo();
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
	mGeometries.clear();
	mMaterials.clear();

	glApp->destroy();
}

void RendererApp::prepareScene() {
	// 如果没有注入自定义管线，则默认使用延迟渲染管线
	if (!mPipeline) {
		auto deferredPipeline = std::make_unique<DeferredRenderPipeline>();
		deferredPipeline->init(mWidth, mHeight);
		mPipeline = std::move(deferredPipeline);
	}
	/*if (!mPipeline) {
		auto defaultPipeline = std::make_unique<DefaultRenderPipeline>();
		defaultPipeline->init(mWidth, mHeight);
		mPipeline = std::move(defaultPipeline);
	}*/
	mMainScene = std::make_unique<Scene>();
	mPostScene = std::make_unique<Scene>();

	// 初始化IBL资源
	prepareIBL();

	// =====================================================================
	// 性能测试场景：20×20 = 400个球体，5种PBR材质循环分配
	// 场景范围 200×200，球体间距10，半径1.2
	// =====================================================================

	// --- 创建5种PBR材质（gold / grass / plastic / rusted_iron / wall）---
	const char* matDirs[] = { "gold", "grass", "plastic", "rusted_iron", "wall" };
	constexpr int MAT_COUNT = 5;
	PbrMaterial* pbrMats[MAT_COUNT]{};

	for (int m = 0; m < MAT_COUNT; ++m) {
		std::string base = std::string("assets/learnResource/textures/pbr/") + matDirs[m] + "/";
		auto* mat = trackMaterial(mMaterials, new PbrMaterial());
		mat->mAlbedo    = Texture::createTexture((base + "albedo.png").c_str(), 0);
		mat->mAO        = Texture::createTexture((base + "ao.png").c_str(), 0);
		mat->mMetallic  = Texture::createTexture((base + "metallic.png").c_str(), 0);
		mat->mNormal    = Texture::createTexture((base + "normal.png").c_str(), 0);
		mat->mRoughness = Texture::createTexture((base + "roughness.png").c_str(), 0);
		mat->mUseNormalMap = true;
		mat->mUseIBL       = true;
		mat->mInstanced    = true; // Add this line so default pipeline uses instanced shader
		mat->mBRDFLUT = mBRDFLUT;
		mat->mIrradianceIndirect = mIrradianceMap;
		mat->mPrefilteredMap = mPrefilteredMap;

		pbrMats[m] = mat;
	}

	// --- 超大地面（单独一个非实例化材质） ---
	auto* planeMat = trackMaterial(mMaterials, new PbrMaterial());
	std::string planeBase = "assets/learnResource/textures/pbr/grass/";
	planeMat->mAlbedo = Texture::createTexture((planeBase + "albedo.png").c_str(), 0);
	planeMat->mAO = Texture::createTexture((planeBase + "ao.png").c_str(), 0);
	planeMat->mMetallic = Texture::createTexture((planeBase + "metallic.png").c_str(), 0);
	planeMat->mNormal = Texture::createTexture((planeBase + "normal.png").c_str(), 0);
	planeMat->mRoughness = Texture::createTexture((planeBase + "roughness.png").c_str(), 0);
	planeMat->mUseNormalMap = true;
	planeMat->mUseIBL = false;
	planeMat->mInstanced = false;

	auto planeGeo = trackGeometry(mGeometries, Geometry::createPlane(300.0f, 300.0f));
	auto planeMesh = new Mesh(planeGeo, planeMat); // 独立材质
	planeMesh->setAngleX(-90.0f);
	planeMesh->setPosition(glm::vec3(0.0f, -2.0f, 0.0f));
	mMainScene->addChild(planeMesh);

	// --- 生成 20×20 球体网格 ---
	constexpr int   GRID_SIZE = 20;
	constexpr float SPACING   = 10.0f;
	constexpr float RADIUS    = 1.2f;
	const float halfExtent = (GRID_SIZE - 1) * SPACING * 0.5f;

	auto sharedSphereGeo = trackGeometry(mGeometries, Geometry::createSphere(RADIUS));

	// Initialize instanced mesh arrays
	std::vector<InstancedMesh*> instancedSpheres(MAT_COUNT);
	for (int i = 0; i < MAT_COUNT; ++i) {
		unsigned int count = (GRID_SIZE * GRID_SIZE) / MAT_COUNT + 1; // max possible
		instancedSpheres[i] = new InstancedMesh(sharedSphereGeo, pbrMats[i], count);
		// Reset count to use it as index
		instancedSpheres[i]->mInstanceCount = 0; 
	}

	for (int row = 0; row < GRID_SIZE; ++row) {
		for (int col = 0; col < GRID_SIZE; ++col) {
			int matIdx = (row * GRID_SIZE + col) % MAT_COUNT;
			float x = col * SPACING - halfExtent;
			float z = row * SPACING - halfExtent;
			float y = 0.0f;

			InstancedMesh* im = instancedSpheres[matIdx];
			im->mInstanceMatrices[im->mInstanceCount++] = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
		}
	}

	for (int i = 0; i < MAT_COUNT; ++i) {
		instancedSpheres[i]->updateMatrices();
		mMainScene->addChild(instancedSpheres[i]);
		mDynamicObjects.push_back(instancedSpheres[i]);
	}

	// 天空盒
	auto skyGeo = Geometry::createBox(1.0f);
	auto skyMat = new CubeMaterial();
	skyMat->mDiffuse = Texture::createExrTexture("assets/textures/pbr/qwantani_dusk_1_4k.exr");
	auto skyMesh = new Mesh(skyGeo, skyMat);
	mMainScene->addChild(skyMesh);

	// 后处理屏幕Pass
	auto sgeo = Geometry::createScreenPlane();
	mScreenMat = new ScreenMaterial();
	mScreenMat->mScreenTexture = mPipeline->getResolveColorAttachment();
	auto smesh = new Mesh(sgeo, mScreenMat);
	mPostScene->addChild(smesh);

	// 初始化RenderContext中不随帧变化的字段
	mRenderCtx.clearColor    = glm::vec3(0.0f);
	mRenderCtx.ambientColor  = glm::vec3(0.2f);
	mRenderCtx.renderModeIdx = 0;

	// 设置IBL资源到RenderContext
	mRenderCtx.iblIrradianceMap  = mIrradianceMap;
	mRenderCtx.iblPrefilteredMap = mPrefilteredMap;
	mRenderCtx.iblBRDFLUT       = mBRDFLUT;

	// =====================================================================
	// 方向光 —— 高强度、低角度，产生明显的阴影和高光
	// =====================================================================
	mDirLight = new DirectionalLight();
	mDirLight->setPosition(glm::vec3(50.0f, 80.0f, 50.0f));
	mDirLight->setAngleX(-45.0f);
	mDirLight->setAngleY(45.0f);
	mDirLight->mColor = glm::vec3(1.0f, 0.95f, 0.85f);  // 暖白色
	mDirLight->mIntensity = 3.0f;
	mDirLight->mSpecularIntensity = 1.0f;

	// =====================================================================
	// 8个点光源 —— 分布在场景四周和中心，彩色高亮度
	// =====================================================================
	glm::vec3 lightPositions[] = {
		glm::vec3(-70.0f, 15.0f, -70.0f),
		glm::vec3(70.0f, 15.0f, -70.0f),
		glm::vec3(-70.0f, 15.0f,  70.0f),
		glm::vec3(70.0f, 15.0f,  70.0f),
		glm::vec3(0.0f, 20.0f,   0.0f),
		glm::vec3(-40.0f, 12.0f,   0.0f),
		glm::vec3(40.0f, 12.0f,   0.0f),
		glm::vec3(0.0f, 12.0f,  40.0f),
	};
	glm::vec3 lightColors[] = {
		glm::vec3(200.0f,  50.0f,  50.0f),   // 红
		glm::vec3(50.0f, 200.0f,  50.0f),   // 绿
		glm::vec3(50.0f,  50.0f, 200.0f),   // 蓝
		glm::vec3(200.0f, 200.0f,  50.0f),   // 黄
		glm::vec3(300.0f, 300.0f, 300.0f),   // 白（中心，最亮）
		glm::vec3(200.0f,  50.0f, 200.0f),   // 品红
		glm::vec3(50.0f, 200.0f, 200.0f),   // 青
		glm::vec3(200.0f, 150.0f,  50.0f),   // 橙
	};
	for (int i = 0; i < 8; ++i) {
		auto* pointLight = new PointLight();
		pointLight->setPosition(lightPositions[i]);
		pointLight->mColor = lightColors[i];
		mPointLights.push_back(pointLight);
	}

	// 将IBL资源传递给延迟管线
	auto* deferredPL = dynamic_cast<DeferredRenderPipeline*>(mPipeline.get());
	if (deferredPL) {
		deferredPL->setIBLResources(mIrradianceMap, mPrefilteredMap, mBRDFLUT);
	}
}

void RendererApp::prepareForwardScene()
{
	mPipeline = std::make_unique<DefaultRenderPipeline>();
	mPipeline->init(mWidth,mHeight);

	mMainScene = std::make_unique<Scene>();
	mPostScene = std::make_unique<Scene>();

	// 初始化IBL资源
	prepareIBL();

	const char* matDirs[] = { "gold", "grass", "plastic", "rusted_iron", "wall" };
	constexpr int MAT_COUNT = 5;
	PbrMaterial* pbrMats[MAT_COUNT]{};

	for (int m = 0; m < MAT_COUNT; ++m) {
		std::string base = std::string("assets/learnResource/textures/pbr/") + matDirs[m] + "/";
		auto* mat = trackMaterial(mMaterials, new PbrMaterial());
		mat->mAlbedo = Texture::createTexture((base + "albedo.png").c_str(), 0);
		mat->mAO = Texture::createTexture((base + "ao.png").c_str(), 0);
		mat->mMetallic = Texture::createTexture((base + "metallic.png").c_str(), 0);
		mat->mNormal = Texture::createTexture((base + "normal.png").c_str(), 0);
		mat->mRoughness = Texture::createTexture((base + "roughness.png").c_str(), 0);
		mat->mIrradianceIndirect = mIrradianceMap;
		mat->mPrefilteredMap = mPrefilteredMap;
		mat->mBRDFLUT = mBRDFLUT;
		mat->mUseNormalMap = true;
		mat->mUseIBL = true;
		pbrMats[m] = mat;
	}

	// 地面 —— 使用PBR材质
	auto planeGeo = trackGeometry(mGeometries, Geometry::createPlane(50.0f, 50.0f));

	auto planeMesh = new Mesh(planeGeo, pbrMats[3]);
	planeMesh->setAngleX(-90.0f);
	planeMesh->setPosition(glm::vec3(0.0f, -2.0f, 0.0f));
	mMainScene->addChild(planeMesh);


	// 初始球体 —— 使用PBR材质
	auto sphereGeo = trackGeometry(mGeometries, Geometry::createSphere(1.0f));
	auto sphereMesh = new Mesh(sphereGeo, pbrMats[0]);
	sphereMesh->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
	mMainScene->addChild(sphereMesh);
	mDynamicObjects.push_back(sphereMesh);

	// 初始立方体 —— 使用PBR材质
	auto boxGeo = trackGeometry(mGeometries, Geometry::createBox(1.5f));
	auto boxMat = trackMaterial(mMaterials, new PbrMaterial());
	auto boxMesh = new Mesh(boxGeo, pbrMats[2]);
	boxMesh->setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
	mMainScene->addChild(boxMesh);
	mDynamicObjects.push_back(boxMesh);

	// 天空盒
	auto skyGeo = trackGeometry(mGeometries, Geometry::createBox(1.0f));
	auto skyMat = trackMaterial(mMaterials, new CubeMaterial());
	skyMat->mDiffuse = Texture::createExrTexture("assets/textures/pbr/qwantani_dusk_1_4k.exr");
	auto skyMesh = new Mesh(skyGeo, skyMat);
	mMainScene->addChild(skyMesh);

	// 后处理屏幕Pass
	auto sgeo = trackGeometry(mGeometries, Geometry::createScreenPlane());
	mScreenMat = trackMaterial(mMaterials, new ScreenMaterial());
	mScreenMat->mScreenTexture = mPipeline->getResolveColorAttachment();
	auto smesh = new Mesh(sgeo, mScreenMat);
	mPostScene->addChild(smesh);

	// 初始化RenderContext中不随帧变化的字段
	mRenderCtx.clearColor = glm::vec3(0.0f);
	mRenderCtx.ambientColor = glm::vec3(0.15f);
	mRenderCtx.renderModeIdx = 0;

	// 设置IBL资源到RenderContext
	mRenderCtx.iblIrradianceMap = mIrradianceMap;
	mRenderCtx.iblPrefilteredMap = mPrefilteredMap;
	mRenderCtx.iblBRDFLUT = mBRDFLUT;

	// 方向光
	mDirLight = new DirectionalLight();
	mDirLight->setPosition(glm::vec3(10.0f, 20.0f, 10.0f));
	mDirLight->setAngleX(-60.0f);
	mDirLight->setAngleY(30.0f);
	mDirLight->mColor = glm::vec3(1.0f);
	mDirLight->mIntensity = 1.5f;
	mDirLight->mSpecularIntensity = 0.5f;

	// 点光源
	glm::vec3 lightPositions[] = {
		glm::vec3(-5.0f,  5.0f, 5.0f),
		glm::vec3(5.0f,  5.0f, 5.0f),
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

void RendererApp::prepareDefferedDemo()
{
	mPipeline = std::make_unique<DeferredRenderPipeline>();
	mPipeline->init(mWidth, mHeight);

	mMainScene = std::make_unique<Scene>();
	mPostScene = std::make_unique<Scene>();

	// 初始化IBL资源
	prepareIBL();

	const char* matDirs[] = { "gold", "grass", "plastic", "rusted_iron", "wall" };
	constexpr int MAT_COUNT = 5;
	PbrMaterial* pbrMats[MAT_COUNT]{};

	for (int m = 0; m < MAT_COUNT; ++m) {
		std::string base = std::string("assets/learnResource/textures/pbr/") + matDirs[m] + "/";
		auto* mat = trackMaterial(mMaterials, new PbrMaterial());
		mat->mAlbedo = Texture::createTexture((base + "albedo.png").c_str(), 0);
		mat->mAO = Texture::createTexture((base + "ao.png").c_str(), 0);
		mat->mMetallic = Texture::createTexture((base + "metallic.png").c_str(), 0);
		mat->mNormal = Texture::createTexture((base + "normal.png").c_str(), 0);
		mat->mRoughness = Texture::createTexture((base + "roughness.png").c_str(), 0);
		mat->mIrradianceIndirect = mIrradianceMap;
		mat->mPrefilteredMap = mPrefilteredMap;
		mat->mBRDFLUT = mBRDFLUT;
		mat->mUseNormalMap = true;
		mat->mUseIBL = true;
		pbrMats[m] = mat;
	}

	// 地面 —— 使用PBR材质
	auto planeGeo = Geometry::createPlane(50.0f, 50.0f);

	auto planeMesh = new Mesh(planeGeo, pbrMats[3]);
	planeMesh->setAngleX(-90.0f);
	planeMesh->setPosition(glm::vec3(0.0f, -2.0f, 0.0f));
	mMainScene->addChild(planeMesh);


	// 初始球体 —— 使用PBR材质
	auto sphereGeo = Geometry::createSphere(1.0f);
	auto sphereMesh = new Mesh(sphereGeo, pbrMats[0]);
	sphereMesh->setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
	mMainScene->addChild(sphereMesh);
	mDynamicObjects.push_back(sphereMesh);

	// 初始立方体 —— 使用PBR材质
	auto boxGeo = Geometry::createBox(1.5f);
	auto boxMat = new PbrMaterial();
	auto boxMesh = new Mesh(boxGeo, pbrMats[2]);
	boxMesh->setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
	mMainScene->addChild(boxMesh);
	mDynamicObjects.push_back(boxMesh);

	// 天空盒
	auto skyGeo = Geometry::createBox(1.0f);
	auto skyMat = new CubeMaterial();
	skyMat->mDiffuse = Texture::createExrTexture("assets/textures/pbr/qwantani_dusk_1_4k.exr");
	auto skyMesh = new Mesh(skyGeo, skyMat);
	mMainScene->addChild(skyMesh);

	// 后处理屏幕Pass
	auto sgeo = Geometry::createScreenPlane();
	mScreenMat = new ScreenMaterial();
	mScreenMat->mScreenTexture = mPipeline->getResolveColorAttachment();
	auto smesh = new Mesh(sgeo, mScreenMat);
	mPostScene->addChild(smesh);

	// 初始化RenderContext中不随帧变化的字段
	mRenderCtx.clearColor = glm::vec3(0.0f);
	mRenderCtx.ambientColor = glm::vec3(0.15f);
	mRenderCtx.renderModeIdx = 0;

	// 设置IBL资源到RenderContext
	mRenderCtx.iblIrradianceMap = mIrradianceMap;
	mRenderCtx.iblPrefilteredMap = mPrefilteredMap;
	mRenderCtx.iblBRDFLUT = mBRDFLUT;

	// 方向光
	mDirLight = new DirectionalLight();
	mDirLight->setPosition(glm::vec3(10.0f, 20.0f, 10.0f));
	mDirLight->setAngleX(-60.0f);
	mDirLight->setAngleY(30.0f);
	mDirLight->mColor = glm::vec3(1.0f);
	mDirLight->mIntensity = 1.5f;
	mDirLight->mSpecularIntensity = 0.5f;

	// 点光源
	glm::vec3 lightPositions[] = {
		glm::vec3(-5.0f,  5.0f, 5.0f),
		glm::vec3(5.0f,  5.0f, 5.0f),
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
	// 将IBL资源传递给延迟管线
	auto* deferredPL = dynamic_cast<DeferredRenderPipeline*>(mPipeline.get());
	if (deferredPL) {
		deferredPL->setIBLResources(mIrradianceMap, mPrefilteredMap, mBRDFLUT);
	}

}

void RendererApp::prepareForwardDemo()
{
	// 如果没有注入自定义管线，则默认使用延迟渲染管线
	if (!mPipeline) {
		auto defaultPipeline = std::make_unique<DefaultRenderPipeline>();
		defaultPipeline->init(mWidth, mHeight);
		mPipeline = std::move(defaultPipeline);
	}
	mMainScene = std::make_unique<Scene>();
	mPostScene = std::make_unique<Scene>();

	// 初始化IBL资源
	prepareIBL();

	// =====================================================================
	// 性能测试场景：20×20 = 400个球体，5种PBR材质循环分配
	// 场景范围 200×200，球体间距10，半径1.2
	// =====================================================================

	// --- 创建5种PBR材质（gold / grass / plastic / rusted_iron / wall）---
	const char* matDirs[] = { "gold", "grass", "plastic", "rusted_iron", "wall" };
	constexpr int MAT_COUNT = 5;
	PbrMaterial* pbrMats[MAT_COUNT]{};

	for (int m = 0; m < MAT_COUNT; ++m) {
		std::string base = std::string("assets/learnResource/textures/pbr/") + matDirs[m] + "/";
		auto* mat = trackMaterial(mMaterials, new PbrMaterial());
		mat->mAlbedo = Texture::createTexture((base + "albedo.png").c_str(), 0);
		mat->mAO = Texture::createTexture((base + "ao.png").c_str(), 0);
		mat->mMetallic = Texture::createTexture((base + "metallic.png").c_str(), 0);
		mat->mNormal = Texture::createTexture((base + "normal.png").c_str(), 0);
		mat->mRoughness = Texture::createTexture((base + "roughness.png").c_str(), 0);
		mat->mUseNormalMap = true;
		mat->mUseIBL = true;
		mat->mBRDFLUT = mBRDFLUT;
		mat->mIrradianceIndirect = mIrradianceMap;
		mat->mPrefilteredMap = mPrefilteredMap;
		mat->mInstanced = true;  // Mark specifically for instanced rendering
		pbrMats[m] = mat;
	}

	// --- 超大地面（单独一个材质，不开启instanced） ---
	auto* planeMat = trackMaterial(mMaterials, new PbrMaterial());
	std::string planeBase = "assets/learnResource/textures/pbr/grass/";
	planeMat->mAlbedo = Texture::createTexture((planeBase + "albedo.png").c_str(), 0);
	planeMat->mAO = Texture::createTexture((planeBase + "ao.png").c_str(), 0);
	planeMat->mMetallic = Texture::createTexture((planeBase + "metallic.png").c_str(), 0);
	planeMat->mNormal = Texture::createTexture((planeBase + "normal.png").c_str(), 0);
	planeMat->mRoughness = Texture::createTexture((planeBase + "roughness.png").c_str(), 0);
	planeMat->mUseNormalMap = true;
	planeMat->mUseIBL = true;
	planeMat->mBRDFLUT = mBRDFLUT;
	planeMat->mIrradianceIndirect = mIrradianceMap;
	planeMat->mPrefilteredMap = mPrefilteredMap;
	planeMat->mInstanced = false;

	auto planeGeo = trackGeometry(mGeometries, Geometry::createPlane(300.0f, 300.0f));
	auto planeMesh = new Mesh(planeGeo, planeMat); // 独立材质
	planeMesh->setAngleX(-90.0f);
	planeMesh->setPosition(glm::vec3(0.0f, -2.0f, 0.0f));
	mMainScene->addChild(planeMesh);

	// --- 生成 20×20 球体网格 ---
	constexpr int   GRID_SIZE = 20;
	constexpr float SPACING = 10.0f;
	constexpr float RADIUS = 1.2f;
	const float halfExtent = (GRID_SIZE - 1) * SPACING * 0.5f;

	auto sharedSphereGeo = trackGeometry(mGeometries, Geometry::createSphere(RADIUS));

	// Initialize instanced mesh arrays
	std::vector<InstancedMesh*> instancedSpheres(MAT_COUNT);
	for (int i = 0; i < MAT_COUNT; ++i) {
		unsigned int count = (GRID_SIZE * GRID_SIZE) / MAT_COUNT + 1; // max possible
		instancedSpheres[i] = new InstancedMesh(sharedSphereGeo, pbrMats[i], count);
		// Reset count to use it as index
		instancedSpheres[i]->mInstanceCount = 0; 
	}

	for (int row = 0; row < GRID_SIZE; ++row) {
		for (int col = 0; col < GRID_SIZE; ++col) {
			int matIdx = (row * GRID_SIZE + col) % MAT_COUNT;
			float x = col * SPACING - halfExtent;
			float z = row * SPACING - halfExtent;
			float y = 0.0f;

			InstancedMesh* im = instancedSpheres[matIdx];
			im->mInstanceMatrices[im->mInstanceCount++] = glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
		}
	}

	for (int i = 0; i < MAT_COUNT; ++i) {
		instancedSpheres[i]->updateMatrices();
		mMainScene->addChild(instancedSpheres[i]);
		mDynamicObjects.push_back(instancedSpheres[i]);
	}

	// 天空盒
	auto skyGeo = Geometry::createBox(1.0f);
	auto skyMat = new CubeMaterial();
	skyMat->mDiffuse = Texture::createExrTexture("assets/textures/pbr/qwantani_dusk_1_4k.exr");
	auto skyMesh = new Mesh(skyGeo, skyMat);
	mMainScene->addChild(skyMesh);

	// 后处理屏幕Pass
	auto sgeo = Geometry::createScreenPlane();
	mScreenMat = new ScreenMaterial();
	mScreenMat->mScreenTexture = mPipeline->getResolveColorAttachment();
	auto smesh = new Mesh(sgeo, mScreenMat);
	mPostScene->addChild(smesh);

	// 初始化RenderContext中不随帧变化的字段
	mRenderCtx.clearColor = glm::vec3(0.0f);
	mRenderCtx.ambientColor = glm::vec3(0.2f);
	mRenderCtx.renderModeIdx = 0;

	// 设置IBL资源到RenderContext
	mRenderCtx.iblIrradianceMap = mIrradianceMap;
	mRenderCtx.iblPrefilteredMap = mPrefilteredMap;
	mRenderCtx.iblBRDFLUT = mBRDFLUT;

	// =====================================================================
	// 方向光 —— 高强度、低角度，产生明显的阴影和高光
	// =====================================================================
	mDirLight = new DirectionalLight();
	mDirLight->setPosition(glm::vec3(50.0f, 80.0f, 50.0f));
	mDirLight->setAngleX(-45.0f);
	mDirLight->setAngleY(45.0f);
	mDirLight->mColor = glm::vec3(1.0f, 0.95f, 0.85f);  // 暖白色
	mDirLight->mIntensity = 3.0f;
	mDirLight->mSpecularIntensity = 1.0f;

	// =====================================================================
	// 8个点光源 —— 分布在场景四周和中心，彩色高亮度
	// =====================================================================
	glm::vec3 lightPositions[] = {
		glm::vec3(-70.0f, 15.0f, -70.0f),
		glm::vec3(70.0f, 15.0f, -70.0f),
		glm::vec3(-70.0f, 15.0f,  70.0f),
		glm::vec3(70.0f, 15.0f,  70.0f),
		glm::vec3(0.0f, 20.0f,   0.0f),
		glm::vec3(-40.0f, 12.0f,   0.0f),
		glm::vec3(40.0f, 12.0f,   0.0f),
		glm::vec3(0.0f, 12.0f,  40.0f),
	};
	glm::vec3 lightColors[] = {
		glm::vec3(200.0f,  50.0f,  50.0f),   // 红
		glm::vec3(50.0f, 200.0f,  50.0f),   // 绿
		glm::vec3(50.0f,  50.0f, 200.0f),   // 蓝
		glm::vec3(200.0f, 200.0f,  50.0f),   // 黄
		glm::vec3(300.0f, 300.0f, 300.0f),   // 白（中心，最亮）
		glm::vec3(200.0f,  50.0f, 200.0f),   // 品红
		glm::vec3(50.0f, 200.0f, 200.0f),   // 青
		glm::vec3(200.0f, 150.0f,  50.0f),   // 橙
	};
	for (int i = 0; i < 8; ++i) {
		auto* pointLight = new PointLight();
		pointLight->setPosition(lightPositions[i]);
		pointLight->mColor = lightColors[i];
		mPointLights.push_back(pointLight);
	}
}

void RendererApp::prepareIBL() {
	// 从HDR环境贴图生成IBL资源
	const std::string hdrPath = "assets/textures/pbr/qwantani_dusk_1_4k.exr";

	// 1. 等距柱状投影 → 立方体贴图
	mEnvCubemap = mIBLGenerator.equirectToCubemap(hdrPath, 512);
	if (mEnvCubemap == 0) {
		std::cout << "[IBL] Warning: env map failed to load, IBL disabled" << std::endl;
		return;
	}

	// 2. 生成漫反射辐照度贴图
	mIrradianceMap = mIBLGenerator.generateIrradiance(mEnvCubemap, 32);

	// 3. 生成镜面反射预滤波环境贴图
	mPrefilteredMap = mIBLGenerator.generatePrefilteredEnvMap(mEnvCubemap, 128, 5);

	// 4. 生成BRDF积分查找表
	mBRDFLUT = mIBLGenerator.generateBRDFLUT(512);

	std::cout << "[IBL] IBL resources initialized" << std::endl;
}

void RendererApp::prepareCamera() {
	mCamera = std::make_unique<PerspectiveCamera>(
		60.0f,
		(float)glApp->getWidth() / (float)glApp->getHeight(),
		0.1f,
		2000.0f
	);
	// 将相机拉远拉高，俯瞰超大场景
	mCamera->mPosition = glm::vec3(0.0f, 60.0f, 120.0f);

	mCameraControl = std::make_unique<GameCameraControl>();
	mCameraControl->setCamera(mCamera.get());
	mCameraControl->setSensitivity(0.4f);
	mCameraControl->setSpeed(0.5f);  // 加快移动速度以适应大场景
}

void RendererApp::buildGuiPanels() {
	// 尝试向下转型到具体的管线类型以访问Bloom等组件
	auto* dp = dynamic_cast<DefaultRenderPipeline*>(mPipeline.get());
	auto* defPL = dynamic_cast<DeferredRenderPipeline*>(mPipeline.get());

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

	// 根据管线类型获取Bloom指针
	Bloom* bloom = nullptr;
	if (dp) {
		bloom = dp->getBloom();
	} else if (defPL) {
		bloom = defPL->getBloom();
	}

	if (bloom) {
		mGuiSystem.addPanel(std::make_unique<RenderingPanel>(
			mRenderCtx.renderModeIdx,
			mRenderCtx.shadowType,
			mRenderCtx.clearColor,
			mRenderCtx.ambientColor,
			mScreenMat,
			bloom,
			mCameraControl.get()
		));
	}
}

// renderImGui() has been removed; GuiSystem::render() takes its place.

void RendererApp::addObject(const std::string& type) {
	Mesh* mesh = nullptr;
	// 使用PBR材质创建新物体

	auto stoneMat = trackMaterial(mMaterials, new PbrMaterial());
	stoneMat->mAlbedo = Texture::createTexture("assets/textures/pbr/slab_tiles_diff_2k.jpg", 0);
	stoneMat->mUseNormalMap = true;
	stoneMat->mNormal = Texture::createTexture("assets/textures/pbr/slab_tiles_nor_2k.jpg", 0);
	stoneMat->mRoughness = Texture::createTexture("assets/textures/pbr/slab_tiles_rough_2k.jpg", 0);
	stoneMat->mIrradianceIndirect = mIrradianceMap;
	stoneMat->mPrefilteredMap = mPrefilteredMap;
	stoneMat->mBRDFLUT = mBRDFLUT;
	stoneMat->mUseIBL = true;
	if (type == "cube") {
		mesh = new Mesh(trackGeometry(mGeometries, Geometry::createBox(1.0f)), stoneMat);
	}
	else {
		mesh = new Mesh(trackGeometry(mGeometries, Geometry::createSphere(0.8f)), stoneMat);
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
