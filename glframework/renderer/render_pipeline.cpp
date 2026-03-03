#include "render_pipeline.h"
#include "renderer.h"
#include "bloom.h"

RenderPipeline::RenderPipeline(int width, int height) {
	mRenderer   = new Renderer();
	mBloom      = new Bloom(width, height);
	mFboMulti   = Framebuffer::createMultiSampleHDRFbo(width, height);
	mFboResolve = Framebuffer::createHDRFbo(width, height);
}

RenderPipeline::~RenderPipeline() {
	delete mRenderer;
	delete mBloom;
	delete mFboMulti;
	delete mFboResolve;
}

void RenderPipeline::setClearColor(glm::vec3 color) {
	mRenderer->setClearColor(color);
}

void RenderPipeline::execute(
	Scene* mainScene,
	Scene* postScene,
	Camera* camera,
	const std::vector<PointLight*>& pointLights
) {
	// Pass 1: render main scene to MSAA HDR FBO
	mRenderer->render(mainScene, camera, pointLights, mFboMulti->mFBO);

	// Pass 2: MSAA resolve
	mRenderer->msaaResolve(mFboMulti, mFboResolve);

	// Pass 3: bloom post-processing
	mBloom->doBloom(mFboResolve);

	// Pass 4: render post-processing scene to screen
	mRenderer->render(postScene, camera, pointLights);
}
