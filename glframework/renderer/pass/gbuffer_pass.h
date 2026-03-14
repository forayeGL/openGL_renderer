#pragma once
#include "render_pass.h"
#include "../../mesh/mesh.h"
#include <vector>
#include <memory>

class Shader;
class UBOManager;

/**
 * @brief GBuffer几何填充Pass（延迟渲染第一阶段）
 * 
 * 将场景中所有不透明物体的几何信息写入GBuffer的多个渲染目标（MRT）：
 *   RT0: 世界空间位置
 *   RT1: 世界空间法线
 *   RT2: 反照率 + 金属度
 *   RT3: 粗糙度 + AO + 自发光标志
 * 
 * 输入：场景中的不透明Mesh列表
 * 输出：填充好的GBuffer FBO
 */
class GBufferPass : public RenderPass {
public:
	GBufferPass();
	~GBufferPass() override;

	void init(int width, int height) override;
	void execute(const RenderContext& ctx) override;
	const char* getName() const override { return "GBufferPass"; }

	/// 获取GBuffer FBO（供后续光照Pass读取）
	Framebuffer* getGBuffer() const { return mGBuffer.get(); }

	/**
	 * @brief 收集场景中的不透明和透明物体
	 * @param scene 主场景
	 * 在execute之前调用，分离不透明和透明物体
	 */
	void collectMeshes(Scene* scene);

	/// 获取不透明物体列表（供阴影Pass使用）
	const std::vector<Mesh*>& getOpaqueMeshes() const { return mOpaqueMeshes; }

	/// 获取透明物体列表（供前向渲染Pass使用）
	const std::vector<Mesh*>& getTransparentMeshes() const { return mTransparentMeshes; }

	/// 获取天空盒物体列表（供前向渲染Pass使用）
	const std::vector<Mesh*>& getSkyboxMeshes() const { return mSkyboxMeshes; }

private:
	/// 递归遍历场景树，收集Mesh到对应列表
	void projectObject(Object* obj);

private:
	std::unique_ptr<Framebuffer> mGBuffer;      ///< GBuffer帧缓冲
	std::vector<Mesh*> mOpaqueMeshes;            ///< 不透明物体列表
	std::vector<Mesh*> mTransparentMeshes;       ///< 透明物体列表
	std::vector<Mesh*> mSkyboxMeshes;            ///< 天空盒物体列表
	int mWidth{ 0 };
	int mHeight{ 0 };
};
