#include "deferred_lighting_pass.h"
#include "../../shader_manager.h"
#include "../../geometry.h"
#include "../ubo_manager.h"

DeferredLightingPass::DeferredLightingPass() = default;
DeferredLightingPass::~DeferredLightingPass() = default;

void DeferredLightingPass::init(int width, int height) {
	mWidth = width;
	mHeight = height;

	// 创建HDR输出FBO（光照计算结果写入此FBO）
	mOutputFBO = std::unique_ptr<Framebuffer>(Framebuffer::createHDRFbo(width, height));

	// 创建全屏四边形（延迟光照使用全屏Pass方式）
	mScreenQuad = Geometry::createScreenPlane();
}

Texture* DeferredLightingPass::getOutputColorTexture() const {
	return mOutputFBO ? mOutputFBO->mColorAttachment : nullptr;
}

void DeferredLightingPass::execute(const RenderContext& ctx) {
	if (!mGBuffer || !mOutputFBO) return;

	// 绑定光照输出FBO
	glBindFramebuffer(GL_FRAMEBUFFER, mOutputFBO->mFBO);
	glViewport(0, 0, mWidth, mHeight);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// 延迟光照阶段不需要深度测试（全屏四边形Pass）
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	// 获取延迟PBR光照Shader
	Shader* shader = ShaderManager::getInstance().getOrCreate(
		"assets/shaders/deferred/deferred_lighting.vert",
		"assets/shaders/deferred/deferred_lighting.frag"
	);
	shader->begin();

	// 绑定UBOs（光源、阴影、渲染设置）
	if (mUBOManager) {
		mUBOManager->bindToShader(shader->getProgram());
	}

	// ==========================================
	// 绑定GBuffer纹理到纹理单元 0-3
	// ==========================================
	const auto& gbufferTextures = mGBuffer->mColorAttachments;

	// RT0: 世界空间位置
	if (gbufferTextures.size() > 0 && gbufferTextures[0]) {
		gbufferTextures[0]->setUnit(0);
		gbufferTextures[0]->bind();
		shader->setInt("gPosition", 0);
	}
	// RT1: 世界空间法线
	if (gbufferTextures.size() > 1 && gbufferTextures[1]) {
		gbufferTextures[1]->setUnit(1);
		gbufferTextures[1]->bind();
		shader->setInt("gNormal", 1);
	}
	// RT2: 反照率 + 金属度
	if (gbufferTextures.size() > 2 && gbufferTextures[2]) {
		gbufferTextures[2]->setUnit(2);
		gbufferTextures[2]->bind();
		shader->setInt("gAlbedo", 2);
	}
	// RT3: 粗糙度 + AO + 自发光
	if (gbufferTextures.size() > 3 && gbufferTextures[3]) {
		gbufferTextures[3]->setUnit(3);
		gbufferTextures[3]->bind();
		shader->setInt("gParam", 3);
	}

	// ==========================================
	// 绑定阴影贴图到纹理单元 4-5+
	// ==========================================

	// 始终将shadowMapSampler指向unit 4，避免默认指向unit 0（GBuffer sampler2D）
	shader->setInt("shadowMapSampler", 4);

	// 方向光阴影贴图
	if (ctx.dirLight && ctx.dirLight->mShadow) {
		auto* shadow = ctx.dirLight->mShadow;
		if (shadow->mRenderTarget && shadow->mRenderTarget->mDepthAttachment) {
			shadow->mRenderTarget->mDepthAttachment->setUnit(4);
			shadow->mRenderTarget->mDepthAttachment->bind();
		}
	}

	// 始终将pointShadowMaps指向非冲突的纹理单元（5-12），
	// 防止samplerCube默认指向unit 0（sampler2D gPosition）导致类型冲突，
	// 该GL错误会残留在错误队列中，在后续CubeMaterial shader创建时触发__debugbreak
	for (int i = 0; i < MAX_POINT_SHADOW; i++) {
		shader->setInt("pointShadowMaps[" + std::to_string(i) + "]", 5 + i);
	}

	// 点光源阴影贴图
	const auto& pointLights = ctx.pointLights ? *ctx.pointLights : std::vector<PointLight*>{};
	for (int i = 0; i < (int)pointLights.size() && i < MAX_POINT_SHADOW; i++) {
		auto* pl = pointLights[i];
		if (pl->mShadow && pl->mShadow->mRenderTarget && pl->mShadow->mRenderTarget->mDepthAttachment) {
			int unit = 5 + i;
			pl->mShadow->mRenderTarget->mDepthAttachment->setUnit(unit);
			pl->mShadow->mRenderTarget->mDepthAttachment->bind();
		}
	}

	// ==========================================
	// 绑定IBL纹理到纹理单元 14-16
	// ==========================================

	// IBL辐照度贴图（漫反射间接光照）
	if (mIrradianceMap) {
		glActiveTexture(GL_TEXTURE14);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mIrradianceMap);
		shader->setInt("irradianceMap", 14);
	}

	// IBL预滤波环境贴图（镜面反射间接光照）
	if (mPrefilteredMap) {
		glActiveTexture(GL_TEXTURE15);
		glBindTexture(GL_TEXTURE_CUBE_MAP, mPrefilteredMap);
		shader->setInt("prefilteredMap", 15);
	}

	// BRDF积分查找表
	if (mBRDFLUT) {
		glActiveTexture(GL_TEXTURE16);
		glBindTexture(GL_TEXTURE_2D, mBRDFLUT);
		shader->setInt("brdfLUT", 16);
	}

	// 设置IBL启用标志
	shader->setInt("useIBL", (mIrradianceMap && mPrefilteredMap && mBRDFLUT) ? 1 : 0);

	// ==========================================
	// 绘制全屏四边形，执行光照计算
	// ==========================================
	glBindVertexArray(mScreenQuad->getVao());
	glDrawElements(GL_TRIANGLES, mScreenQuad->getIndicesCount(), GL_UNSIGNED_INT, 0);
}
