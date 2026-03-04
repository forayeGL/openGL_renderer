#include "render_pipeline.h"
#include "renderer.h"
#include "bloom.h"

RenderPipeline::RenderPipeline(int width, int height)
	: mRenderer(std::make_unique<Renderer>())
	, mBloom(std::make_unique<Bloom>(width, height))
	, mFboMulti(Framebuffer::createMultiSampleHDRFbo(width, height))
	, mFboResolve(Framebuffer::createHDRFbo(width, height))
	, mWidth(width)
	, mHeight(height)
{
}

RenderPipeline::~RenderPipeline() = default;

void RenderPipeline::setClearColor(glm::vec3 color) {
	mRenderer->setClearColor(color);
}

void RenderPipeline::setRenderMode(RenderMode mode) {
	mRenderer->setRenderMode(mode);
}

RenderMode RenderPipeline::getRenderMode() const {
	return mRenderer->getRenderMode();
}

void RenderPipeline::setAmbientColor(glm::vec3 c) {
	mRenderer->setAmbientColor(c);
}

void RenderPipeline::execute(
	Scene* mainScene,
	Scene* postScene,
	Camera* camera,
	DirectionalLight* dirLight,
	const std::vector<PointLight*>& pointLights
) {
	// Pass 1: render main scene to MSAA HDR FBO (shadow passes happen inside)
	mRenderer->render(mainScene, camera, dirLight, pointLights, mFboMulti->mFBO);

	// Pass 2: MSAA resolve
	mRenderer->msaaResolve(mFboMulti.get(), mFboResolve.get());

	// Pass 3: bloom post-processing
	mBloom->doBloom(mFboResolve.get());

	// Pass 4: render post-processing scene to screen
	mRenderer->render(postScene, camera, nullptr, pointLights);
}
