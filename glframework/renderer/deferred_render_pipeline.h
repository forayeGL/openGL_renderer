#pragma once
#include "i_render_pipeline.h"
#include "ubo_manager.h"
#include <memory>

class Renderer;
class Bloom;
class ShadowPass;
class GBufferPass;
class DeferredLightingPass;
class PostProcessPass;
class DebugAxis;
class TemporalAA;

/**
 * @brief 延迟渲染管线
 * 
 * 基于Pass抽象的现代延迟渲染管线，将渲染流程分解为独立的Pass：
 * 
 * 渲染流程：
 *   1. 物体收集阶段：遍历场景树，分离不透明/透明物体
 *   2. ShadowPass：生成方向光和点光源的阴影贴图
 *   3. GBufferPass：将不透明物体的几何+材质信息写入GBuffer（MRT）
 *   4. DeferredLightingPass：从GBuffer读取数据，计算PBR光照+IBL+阴影
 *   5. PostProcessPass：对光照结果进行Bloom等后处理
 *   6. 前向渲染透明物体（与默认管线合并）
 *   7. 后处理屏幕输出
 * 
 * 与DefaultRenderPipeline的关系：
 * - 实现相同的IRenderPipeline接口，可无缝替换
 * - 通过RenderContext传递数据，与应用层解耦
 * 
 * 优势：
 * - 光照计算复杂度与场景物体数量解耦
 * - 多光源场景下性能优于前向渲染
 * - Pass之间通过FBO传递数据，可独立调试和替换
 */
class DeferredRenderPipeline : public IRenderPipeline {
public:
	DeferredRenderPipeline();
	~DeferredRenderPipeline() override;

	void init(int width, int height) override;
    void resize(int width, int height) override;
	void execute(const RenderContext& ctx) override;
	Texture* getResolveColorAttachment() const override;

	/// 获取Bloom对象（供GUI面板调参）
	Bloom* getBloom() const;
	TemporalAA* getTAA() const;

	/// 获取Renderer对象（供GUI面板使用，用于前向渲染透明物体）
	Renderer* getRenderer() const { return mRenderer.get(); }

private:
	/// 天空盒前向渲染（延迟光照后、后处理前）
	void renderSkybox(const RenderContext& ctx);

	/// 透明物体前向合成（延迟光照后、后处理前）
	void renderTransparentMeshes(const RenderContext& ctx);

	/// 各渲染Pass
	std::unique_ptr<ShadowPass>           mShadowPass;
	std::unique_ptr<GBufferPass>          mGBufferPass;
	std::unique_ptr<DeferredLightingPass> mLightingPass;
	std::unique_ptr<PostProcessPass>      mPostProcessPass;

	/// 前向渲染器（用于透明物体和后处理屏幕）
	std::unique_ptr<Renderer>             mRenderer;

	/// 调试坐标系渲染器
	std::unique_ptr<DebugAxis>            mDebugAxis;

	/// UBO管理器
	UBOManager mUBOManager;

	int mWidth{ 0 };
	int mHeight{ 0 };
  bool mHasPrevCameraState{ false };
	glm::vec3 mPrevCameraPosition{ 0.0f };
	glm::vec3 mPrevCameraUp{ 0.0f, 1.0f, 0.0f };
	glm::vec3 mPrevCameraRight{ 1.0f, 0.0f, 0.0f };
};
