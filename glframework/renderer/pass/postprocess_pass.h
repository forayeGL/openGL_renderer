#pragma once
#include "render_pass.h"
#include <memory>

class Bloom;
class Renderer;

/**
 * @brief 后处理Pass
 * 
 * 管线的最终阶段，对光照结果进行后处理：
 * 1. Bloom泛光效果（提取亮度 → 降采样 → 升采样 → 合并）
 * 2. 将处理后的HDR结果输出到resolve FBO
 * 
 * 输入：延迟光照阶段的HDR FBO
 * 输出：后处理完成的FBO（供屏幕材质采样）
 */
class PostProcessPass : public RenderPass {
public:
	PostProcessPass();
	~PostProcessPass() override;

	void init(int width, int height) override;
	void execute(const RenderContext& ctx) override;
	const char* getName() const override { return "PostProcessPass"; }

	/**
	 * @brief 设置输入FBO（光照阶段的输出）
	 * @param fbo HDR光照结果FBO
	 */
	void setInputFBO(Framebuffer* fbo) { mInputFBO = fbo; }

	/// 获取Bloom对象（供GUI面板调参）
	Bloom* getBloom() const { return mBloom.get(); }

	/// 获取后处理结果FBO的颜色附件（供屏幕材质使用）
	Texture* getOutputColorTexture() const;

	/// 获取输出FBO
	Framebuffer* getOutputFBO() const { return mOutputFBO.get(); }

private:
	Framebuffer* mInputFBO{ nullptr };             ///< 输入FBO（不拥有所有权）
	std::unique_ptr<Framebuffer> mOutputFBO;       ///< 后处理输出FBO
	std::unique_ptr<Bloom> mBloom;                  ///< Bloom泛光处理器
	int mWidth{ 0 };
	int mHeight{ 0 };
};
