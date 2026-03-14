#pragma once
#include "render_pass.h"
#include <vector>

class Mesh;
class Shader;

/**
 * @brief 阴影渲染Pass
 * 
 * 负责生成方向光和点光源的阴影贴图。
 * 从主渲染器中解耦出来的独立阴影生成阶段。
 * 
 * 功能：
 * - 方向光的2D阴影贴图（Shadow Map）
 * - 点光源的立方体阴影贴图（Cube Shadow Map）
 * - 自动保存和恢复Viewport/FBO状态
 */
class ShadowPass : public RenderPass {
public:
	ShadowPass();
	~ShadowPass() override;

	void init(int width, int height) override;
	void execute(const RenderContext& ctx) override;
	const char* getName() const override { return "ShadowPass"; }

	/**
	 * @brief 设置需要投射阴影的不透明物体列表
	 * @param meshes 不透明Mesh列表（由管线在GBuffer阶段前收集）
	 */
	void setOpaqueMeshes(const std::vector<Mesh*>& meshes);

private:
	/// 渲染方向光阴影贴图
	void renderDirectionalShadow(const RenderContext& ctx);

	/// 渲染所有点光源的立方体阴影贴图
	void renderPointShadows(const RenderContext& ctx);

private:
	std::vector<Mesh*> mOpaqueMeshes;  ///< 需要投射阴影的物体
};
