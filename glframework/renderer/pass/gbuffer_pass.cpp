#include "gbuffer_pass.h"
#include "../../shader_manager.h"
#include "../../scene.h"
#include "../../material/advanced/pbrMaterial.h"

GBufferPass::GBufferPass() = default;
GBufferPass::~GBufferPass() = default;

void GBufferPass::init(int width, int height) {
	mWidth = width;
	mHeight = height;
	// 创建GBuffer FBO（4个颜色附件 + 深度模板附件）
	mGBuffer = std::unique_ptr<Framebuffer>(Framebuffer::createGBufferFbo(width, height));
}

void GBufferPass::collectMeshes(Scene* scene) {
	mOpaqueMeshes.clear();
	mTransparentMeshes.clear();
	mSkyboxMeshes.clear();
	projectObject(scene);
}

void GBufferPass::projectObject(Object* obj) {
	if (obj->getType() == ObjectType::Mesh || obj->getType() == ObjectType::InstancedMesh) {
		Mesh* mesh = (Mesh*)obj;
		auto material = mesh->mMaterial;

		if (material->mType == MaterialType::CubeMaterial) {
			// 天空盒单独收集，不参与GBuffer填充
			mSkyboxMeshes.push_back(mesh);
		} else if (material->mBlend) {
			mTransparentMeshes.push_back(mesh);
		} else {
			mOpaqueMeshes.push_back(mesh);
		}
	}

	auto& children = obj->getChildren();
	for (size_t i = 0; i < children.size(); i++) {
		projectObject(children[i]);
	}
}

void GBufferPass::execute(const RenderContext& ctx) {
	// 绑定GBuffer FBO
	glBindFramebuffer(GL_FRAMEBUFFER, mGBuffer->mFBO);
	glViewport(0, 0, mWidth, mHeight);

	// 清空所有附件
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	// 开启深度测试，关闭混合（GBuffer阶段不需要混合）
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glDisable(GL_STENCIL_TEST);

	// 获取GBuffer填充Shader
	Shader* shader = ShaderManager::getInstance().getOrCreate(
		"assets/shaders/deferred/gbuffer.vert",
		"assets/shaders/deferred/gbuffer.frag"
	);
	shader->begin();

	const auto emptyLights = std::vector<PointLight*>{};
	const auto& lights = ctx.pointLights ? *ctx.pointLights : emptyLights;

	if (ctx.renderModeIdx == 1) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// 逐物体渲染到GBuffer（仅PBR材质）
	for (auto* mesh : mOpaqueMeshes) {
		auto* material = mesh->mMaterial;

		// 只处理PBR材质类型的物体
		if (material->mType != MaterialType::PbrMaterial &&
			material->mType != MaterialType::DeferredPbrMaterial) {
			continue;
		}

		// PBR材质的applyUniforms会设置GBuffer shader所需的所有uniform
		material->applyUniforms(shader, mesh, ctx.camera, lights);

		glBindVertexArray(mesh->mGeometry->getVao());
		glDrawElements(GL_TRIANGLES, mesh->mGeometry->getIndicesCount(), GL_UNSIGNED_INT, 0);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
