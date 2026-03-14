#include "default_render_pipeline.h"
#include "deferred_render_pipeline.h"
#include "renderer.h"
#include "bloom.h"
#include "debug_axis.h"
#include "pass/shadow_pass.h"
#include "pass/gbuffer_pass.h"
#include "pass/deferred_lighting_pass.h"
#include "pass/postprocess_pass.h"
#include "../shader_manager.h"

DeferredRenderPipeline::DeferredRenderPipeline() = default;
DeferredRenderPipeline::~DeferredRenderPipeline() = default;

void DeferredRenderPipeline::init(int width, int height) {
	mShadowPass      = std::make_unique<ShadowPass>();
	mGBufferPass     = std::make_unique<GBufferPass>();
	mLightingPass    = std::make_unique<DeferredLightingPass>();
	mPostProcessPass = std::make_unique<PostProcessPass>();
	mRenderer        = std::make_unique<Renderer>();
	mDebugAxis       = std::make_unique<DebugAxis>();

	mWidth  = width;
	mHeight = height;

	// 初始化UBO管理器
	mUBOManager.init();

	// ==========================================
	// 创建并初始化各Pass
	// ==========================================

	// 阴影Pass
	mShadowPass->init(width, height);

	// GBuffer填充Pass
	mGBufferPass->init(width, height);

	// 延迟光照Pass
	mLightingPass->init(width, height);
	mLightingPass->setUBOManager(&mUBOManager);

	// 后处理Pass
	mPostProcessPass->init(width, height);

	// 前向渲染器（用于透明物体和后处理屏幕）
	mRenderer = std::make_unique<Renderer>();
}

Texture* DeferredRenderPipeline::getResolveColorAttachment() const {
	// 返回后处理结果的颜色附件
	if (mPostProcessPass) {
		return mPostProcessPass->getOutputColorTexture();
	}
	return nullptr;
}

Bloom* DeferredRenderPipeline::getBloom() const {
	return mPostProcessPass ? mPostProcessPass->getBloom() : nullptr;
}

void DeferredRenderPipeline::setIBLResources(
	GLuint irradiance, GLuint prefiltered, GLuint brdfLUT
) {
	if (mLightingPass) {
		mLightingPass->setIrradianceMap(irradiance);
		mLightingPass->setPrefilteredMap(prefiltered);
		mLightingPass->setBRDFLUT(brdfLUT);
	}
}

void DeferredRenderPipeline::execute(const RenderContext& ctx) {
	const auto& lights = ctx.pointLights ? *ctx.pointLights : std::vector<PointLight*>{};

	// 清除初始化阶段可能残留的GL错误（IBL生成等）
	while (glGetError() != GL_NO_ERROR) {}

	// ==========================================
	// 阶段0：收集场景中的物体（不透明、透明、天空盒）
	// ==========================================
	mGBufferPass->collectMeshes(ctx.mainScene);
	const auto& opaqueMeshes = mGBufferPass->getOpaqueMeshes();

	// 保存原始Viewport状态
	GLint savedViewport[4];
	glGetIntegerv(GL_VIEWPORT, savedViewport);

	// ==========================================
	// 阶段1：阴影Pass - 生成阴影贴图
	// ==========================================
	mShadowPass->setOpaqueMeshes(opaqueMeshes);
	mShadowPass->execute(ctx);

	// 恢复Viewport
	glViewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);

	// ==========================================
	// 阶段2：更新UBO数据
	// ==========================================
	mUBOManager.updateLights(ctx.dirLight, lights);
	mUBOManager.updateShadow(ctx.dirLight, lights);
	mUBOManager.updateRenderSettings(
		ctx.camera,
		static_cast<RenderMode>(ctx.renderModeIdx),
		ctx.shadowType,
		ctx.ambientColor
	);

	// ==========================================
	// 阶段3：GBuffer填充 - 将几何信息写入MRT
	// ==========================================
	mGBufferPass->execute(ctx);

	// 恢复Viewport
	glViewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);

	// ==========================================
	// 阶段4：延迟光照 - PBR光照计算
	// ==========================================
	mLightingPass->setGBuffer(mGBufferPass->getGBuffer());
	mLightingPass->execute(ctx);

	// ==========================================
	// 阶段5：天空盒前向渲染
	// ==========================================
	renderSkybox(ctx);

	if (ctx.enableAxis) {
		mDebugAxis->render(ctx, mLightingPass->getOutputFBO()->mFBO, mWidth, mHeight);
	}

	// ==========================================
	// 阶段6：后处理 - Bloom等
	// ==========================================
	mPostProcessPass->setInputFBO(mLightingPass->getOutputFBO());
	mPostProcessPass->execute(ctx);

	// ==========================================
	// 阶段7：后处理屏幕Pass（色调映射 + gamma校正）
	// ==========================================
	mRenderer->setClearColor(ctx.clearColor);
	mRenderer->setRenderMode(RenderMode::Fill);
	mRenderer->setAmbientColor(ctx.ambientColor);

	// 渲染后处理屏幕（ScreenMaterial读取后处理结果纹理）
	mRenderer->render(ctx.postScene, ctx.camera, nullptr, lights);
}

void DeferredRenderPipeline::renderSkybox(const RenderContext& ctx) {
	const auto& skyboxMeshes = mGBufferPass->getSkyboxMeshes();
	if (skyboxMeshes.empty()) return;

	auto* outputFBO = mLightingPass->getOutputFBO();
	auto* gbuffer = mGBufferPass->getGBuffer();

	// 将GBuffer深度复制到光照输出FBO，使天空盒在已有几何体后面正确渲染
	glBindFramebuffer(GL_READ_FRAMEBUFFER, gbuffer->mFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, outputFBO->mFBO);
	glBlitFramebuffer(
		0, 0, mWidth, mHeight,
		0, 0, mWidth, mHeight,
		GL_DEPTH_BUFFER_BIT, GL_NEAREST
	);

	// 绑定光照输出FBO进行天空盒渲染
	glBindFramebuffer(GL_FRAMEBUFFER, outputFBO->mFBO);
	glViewport(0, 0, mWidth, mHeight);

	// 天空盒使用 gl_Position.xyww 使深度 = 1.0，GL_LEQUAL 使其仅在无几何体处通过
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);

	const auto emptyLights = std::vector<PointLight*>{};

	for (auto* mesh : skyboxMeshes) {
		auto* material = mesh->mMaterial;

		Shader* shader = ShaderManager::getInstance().getOrCreate(
			material->getVertexShaderPath(),
			material->getFragmentShaderPath()
		);
		shader->begin();

		material->applyUniforms(shader, mesh, ctx.camera, emptyLights);

		glBindVertexArray(mesh->mGeometry->getVao());
		glDrawElements(GL_TRIANGLES, mesh->mGeometry->getIndicesCount(), GL_UNSIGNED_INT, 0);
	}

	// 恢复深度状态
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LESS);
}
