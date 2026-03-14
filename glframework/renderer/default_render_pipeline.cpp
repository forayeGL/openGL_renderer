#include "default_render_pipeline.h"
#include "renderer.h"
#include "bloom.h"
#include "debug_axis_pass.h"

// Constructors defined here so Renderer/Bloom/DebugAxisPass are complete for unique_ptr deleters.
DefaultRenderPipeline::DefaultRenderPipeline() = default;
DefaultRenderPipeline::~DefaultRenderPipeline() = default;

void DefaultRenderPipeline::init(int width, int height) {
	mWidth  = width;
	mHeight = height;
	mRenderer      = std::make_unique<Renderer>();
	mBloom         = std::make_unique<Bloom>(width, height);
	mDebugAxisPass = std::make_unique<DebugAxisPass>();
	mDebugAxisPass->init();
	mFboMulti  = std::unique_ptr<Framebuffer>(Framebuffer::createMultiSampleHDRFbo(width, height));
	mFboResolve= std::unique_ptr<Framebuffer>(Framebuffer::createHDRFbo(width, height));
}

Texture* DefaultRenderPipeline::getResolveColorAttachment() const {
	return mFboResolve->mColorAttachment;
}

void DefaultRenderPipeline::execute(const RenderContext& ctx) {
	// Apply per-frame settings from RenderContext
	mRenderer->setClearColor(ctx.clearColor);
	mRenderer->setRenderMode(static_cast<RenderMode>(ctx.renderModeIdx));
	mRenderer->setAmbientColor(ctx.ambientColor);

	const auto& lights = ctx.pointLights ? *ctx.pointLights : std::vector<PointLight*>{};

	// Pass 1: shadow + main scene → MSAA HDR FBO
	mRenderer->render(ctx.mainScene, ctx.camera, ctx.dirLight, lights, mFboMulti->mFBO);

	// Pass 2: optional debug axis / grid overlay (rendered into the same MSAA HDR FBO)
	if (ctx.showDebugAxis) {
		mDebugAxisPass->execute(ctx.camera, mFboMulti->mFBO, mWidth, mHeight);
	}

	// Pass 3: MSAA resolve
	mRenderer->msaaResolve(mFboMulti.get(), mFboResolve.get());

	// Pass 4: bloom post-processing
	mBloom->doBloom(mFboResolve.get());

	// Pass 5: post scene → screen (no shadow, no lights)
	mRenderer->render(ctx.postScene, ctx.camera, nullptr, lights);
}
