#include "shadow_pass.h"
#include "../../mesh/mesh.h"
#include "../../shader_manager.h"
#include "../../light/shadow/directionalLightShadow.h"
#include "../../light/shadow/pointLightShadow.h"

ShadowPass::ShadowPass() = default;
ShadowPass::~ShadowPass() = default;

void ShadowPass::init(int /*width*/, int /*height*/) {
	// 阴影Pass不需要自己创建FBO，使用各光源Shadow对象自带的FBO
}

void ShadowPass::setOpaqueMeshes(const std::vector<Mesh*>& meshes) {
	mOpaqueMeshes = meshes;
}

void ShadowPass::execute(const RenderContext& ctx) {
	// 渲染方向光阴影
	renderDirectionalShadow(ctx);

	// 渲染所有点光源阴影
	renderPointShadows(ctx);
}

void ShadowPass::renderDirectionalShadow(const RenderContext& ctx) {
	if (!ctx.dirLight || !ctx.dirLight->mShadow) return;

	auto* shadow = static_cast<DirectionalLightShadow*>(ctx.dirLight->mShadow);
	auto* fbo = shadow->mRenderTarget;

	// 绑定阴影FBO并设置Viewport
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->mFBO);
	glViewport(0, 0, fbo->mWidth, fbo->mHeight);
	glClear(GL_DEPTH_BUFFER_BIT);

	// 设置深度测试状态
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	// 计算光源空间变换矩阵
	auto lightMatrix = shadow->getLightMatrix(ctx.dirLight->getModelMatrix());

	// 获取阴影Shader
	Shader* shader = ShaderManager::getInstance().getOrCreate(
		"assets/shaders/shadow/shadow.vert",
		"assets/shaders/shadow/shadow.frag"
	);
	shader->begin();
	shader->setMatrix4x4("lightMatrix", lightMatrix);

	// 逐物体渲染深度
	for (auto* mesh : mOpaqueMeshes) {
		shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
		glBindVertexArray(mesh->mGeometry->getVao());
		glDrawElements(GL_TRIANGLES, mesh->mGeometry->getIndicesCount(), GL_UNSIGNED_INT, 0);
	}
}

void ShadowPass::renderPointShadows(const RenderContext& ctx) {
	if (!ctx.pointLights) return;
	const auto& pointLights = *ctx.pointLights;

	for (auto* pl : pointLights) {
		if (!pl->mShadow) continue;

		auto* shadow = static_cast<PointLightShadow*>(pl->mShadow);
		auto* fbo = shadow->mRenderTarget;
		auto lightMatrices = shadow->getLightMatrices(pl->getPosition());

		// 获取点光源阴影Shader
		Shader* shader = ShaderManager::getInstance().getOrCreate(
			"assets/shaders/shadow/shadowDistance.vert",
			"assets/shaders/shadow/shadowDistance.frag"
		);

		shader->begin();
		shader->setFloat("far", shadow->mCamera->mFar);
		shader->setVector3("lightPosition", pl->getPosition());

		// 设置深度测试状态
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		// 渲染立方体贴图的6个面
		for (int face = 0; face < 6; face++) {
			glBindFramebuffer(GL_FRAMEBUFFER, fbo->mFBO);
			glFramebufferTexture2D(
				GL_FRAMEBUFFER,
				GL_DEPTH_ATTACHMENT,
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
				fbo->mDepthAttachment->getTexture(),
				0
			);
			glViewport(0, 0, fbo->mWidth, fbo->mHeight);
			glClear(GL_DEPTH_BUFFER_BIT);

			shader->setMatrix4x4("lightMatrix", lightMatrices[face]);

			for (auto* mesh : mOpaqueMeshes) {
				shader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
				glBindVertexArray(mesh->mGeometry->getVao());
				glDrawElements(GL_TRIANGLES, mesh->mGeometry->getIndicesCount(), GL_UNSIGNED_INT, 0);
			}
		}
	}
}
