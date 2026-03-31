#include "default_render_pipeline.h"
#include "renderer.h"
#include "bloom.h"
#include "debug_axis.h"
#include "temporal_aa.h"
#include "../light/shadow/directionalLightShadow.h"
#include "../light/shadow/directionalLightCSMShadow.h"

namespace {
	constexpr float kCameraStateEpsilon = 1e-5f;

	bool isCameraChanged(const Camera* camera, const glm::vec3& prevPos, const glm::vec3& prevUp, const glm::vec3& prevRight) {
		if (!camera) return true;
		return glm::length(camera->mPosition - prevPos) > kCameraStateEpsilon
			|| glm::length(camera->mUp - prevUp) > kCameraStateEpsilon
			|| glm::length(camera->mRight - prevRight) > kCameraStateEpsilon;
	}

	void ensureDirectionalShadowMode(DirectionalLight* dirLight, int shadowType) {
		if (!dirLight) return;

		const bool wantCSM = (shadowType == 2);
		const bool isCSM = dynamic_cast<DirectionalLightCSMShadow*>(dirLight->mShadow) != nullptr;

		if (wantCSM == isCSM) return;

		delete dirLight->mShadow;
		dirLight->mShadow = wantCSM
			? static_cast<Shadow*>(new DirectionalLightCSMShadow())
			: static_cast<Shadow*>(new DirectionalLightShadow());
	}
}

// Constructors defined here so Renderer/Bloom are complete for unique_ptr deleters.
DefaultRenderPipeline::DefaultRenderPipeline() = default;
DefaultRenderPipeline::~DefaultRenderPipeline() = default;

void DefaultRenderPipeline::init(int width, int height) {
	mWidth  = width;
	mHeight = height;
  if (!mRenderer) {
		mRenderer = std::make_unique<Renderer>();
	}
	mBloom     = std::make_unique<Bloom>(width, height);
	mTAA       = std::make_unique<TemporalAA>(width, height);
	mFboMulti  = std::unique_ptr<Framebuffer>(Framebuffer::createMultiSampleHDRFbo(width, height));
    mFboPreTAA = std::unique_ptr<Framebuffer>(Framebuffer::createHDRFbo(width, height));
	mFboResolve= std::unique_ptr<Framebuffer>(Framebuffer::createHDRFbo(width, height));
 if (!mDebugAxis) {
		mDebugAxis = std::make_unique<DebugAxis>();
	}
	mUBOManager.init();
   mHasPrevCameraState = false;
}

void DefaultRenderPipeline::resize(int width, int height) {
	if (width <= 0 || height <= 0) return;
	if (width == mWidth && height == mHeight) return;

	mWidth = width;
	mHeight = height;
	mBloom = std::make_unique<Bloom>(width, height);
	mTAA = std::make_unique<TemporalAA>(width, height);
	mFboMulti = std::unique_ptr<Framebuffer>(Framebuffer::createMultiSampleHDRFbo(width, height));
	mFboPreTAA = std::unique_ptr<Framebuffer>(Framebuffer::createHDRFbo(width, height));
	mFboResolve = std::unique_ptr<Framebuffer>(Framebuffer::createHDRFbo(width, height));
	mHasPrevCameraState = false;
}

Texture* DefaultRenderPipeline::getResolveColorAttachment() const {
	return mFboResolve->mColorAttachment;
}

void DefaultRenderPipeline::execute(const RenderContext& ctx) {
	// 清除初始化阶段可能残留的GL错误（IBL生成等）
	while (glGetError() != GL_NO_ERROR) {}

	// Apply per-frame settings from RenderContext
	mRenderer->setClearColor(ctx.clearColor);
	mRenderer->setRenderMode(static_cast<RenderMode>(ctx.renderModeIdx));
	mRenderer->setShadowType(ctx.shadowType);
	mRenderer->setAmbientColor(ctx.ambientColor);

	const auto& lights = ctx.pointLights ? *ctx.pointLights : std::vector<PointLight*>{};
   ensureDirectionalShadowMode(ctx.dirLight, ctx.shadowType);
 const bool taaEnabled = ctx.enableTAA && mTAA && ctx.camera;
	if (taaEnabled) {
		ctx.camera->setProjectionJitter(mTAA->consumeJitterNdc());
	} else if (ctx.camera) {
		ctx.camera->clearProjectionJitter();
	}

	const bool cameraChanged = !mHasPrevCameraState
		|| isCameraChanged(ctx.camera, mPrevCameraPosition, mPrevCameraUp, mPrevCameraRight);
	const bool resetTAAHistory = ctx.taaResetHistory || cameraChanged;

	mUBOManager.updateLights(ctx.dirLight, lights);
 mUBOManager.updateShadow(ctx.dirLight, ctx.camera, lights);
	mUBOManager.updateRenderSettings(
		ctx.camera,
		static_cast<RenderMode>(ctx.renderModeIdx),
		ctx.shadowType,
		ctx.ambientColor
	);

	// Pass 1: shadow + main scene → MSAA HDR FBO
	mRenderer->render(ctx.mainScene, ctx.camera, ctx.dirLight, lights, mFboMulti->mFBO);

	if (ctx.enableAxis) {
		mDebugAxis->render(ctx, mFboMulti->mFBO, mWidth, mHeight);
	}

	// Pass 2: MSAA resolve
 if (taaEnabled) {
		mRenderer->msaaResolve(mFboMulti.get(), mFboPreTAA.get());
		mTAA->resolve(
			mFboPreTAA.get(),
			mFboResolve.get(),
			ctx.taaBlendFactor,
			ctx.taaNeighborhoodClamp,
			resetTAAHistory
		);
	} else {
		if (mTAA) {
			mTAA->resetHistory();
		}
		mRenderer->msaaResolve(mFboMulti.get(), mFboResolve.get());
	}

	// Pass 3: bloom post-processing
	mBloom->doBloom(mFboResolve.get());

	// Pass 4: post scene → screen (no shadow, no lights)
	mRenderer->setRenderMode(RenderMode::Fill);
  const auto emptyLights = std::vector<PointLight*>{};
	mRenderer->render(ctx.postScene, ctx.camera, nullptr, emptyLights);

	if (ctx.camera) {
		mPrevCameraPosition = ctx.camera->mPosition;
		mPrevCameraUp = ctx.camera->mUp;
		mPrevCameraRight = ctx.camera->mRight;
		mHasPrevCameraState = true;
		ctx.camera->clearProjectionJitter();
	}
}
