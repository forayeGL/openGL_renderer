#include "default_render_pipeline.h"
#include "renderer.h"
#include "bloom.h"
#include "debug_axis.h"

// Constructors defined here so Renderer/Bloom are complete for unique_ptr deleters.
DefaultRenderPipeline::DefaultRenderPipeline() = default;
DefaultRenderPipeline::~DefaultRenderPipeline() = default;

void DefaultRenderPipeline::init(int width, int height) {
	mWidth  = width;
	mHeight = height;
	mRenderer  = std::make_unique<Renderer>();
	mBloom     = std::make_unique<Bloom>(width, height);
	mFboMulti  = std::unique_ptr<Framebuffer>(Framebuffer::createMultiSampleHDRFbo(width, height));
	mFboResolve= std::unique_ptr<Framebuffer>(Framebuffer::createHDRFbo(width, height));
	mDebugAxis = std::make_unique<DebugAxis>();
	mUBOManager.init();
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
	mUBOManager.updateLights(ctx.dirLight, lights);
	mUBOManager.updateShadow(ctx.dirLight, lights);
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
	mRenderer->msaaResolve(mFboMulti.get(), mFboResolve.get());

	// Pass 3: bloom post-processing
	mBloom->doBloom(mFboResolve.get());

	// Pass 4: post scene → screen (no shadow, no lights)
	mRenderer->setRenderMode(RenderMode::Fill);
	mRenderer->render(ctx.postScene, ctx.camera, nullptr, lights);
}
