#pragma once
#include "render_pass.h"
#include <memory>

class Shader;
class Geometry;
class UBOManager;

/**
 * @brief 延迟渲染PBR光照Pass
 * 
 * 延迟渲染的第二阶段：从GBuffer中读取几何信息，
 * 计算PBR直接光照（方向光+点光源）、IBL间接光照和阴影。
 * 
 * 输入：GBuffer的4个纹理附件 + 阴影贴图 + IBL环境贴图
 * 输出：HDR光照结果 FBO
 * 
 * 光照模型：
 * - Cook-Torrance BRDF 直接光照
 * - IBL漫反射（irradiance map）
 * - IBL镜面反射（prefiltered env map + BRDF LUT）
 * - 方向光PCF阴影
 * - 点光源立方体阴影
 */
class DeferredLightingPass : public RenderPass {
public:
	DeferredLightingPass();
	~DeferredLightingPass() override;

	void init(int width, int height) override;
	void execute(const RenderContext& ctx) override;
	const char* getName() const override { return "DeferredLightingPass"; }

	/**
	 * @brief 设置GBuffer FBO引用（由管线传入）
	 * @param gbuffer 已填充的GBuffer帧缓冲
	 */
	void setGBuffer(Framebuffer* gbuffer) { mGBuffer = gbuffer; }

	/**
	 * @brief 设置UBO管理器引用
	 * @param ubo UBOManager指针
	 */
	void setUBOManager(UBOManager* ubo) { mUBOManager = ubo; }

	/// 获取光照输出FBO
	Framebuffer* getOutputFBO() const { return mOutputFBO.get(); }

	/// 获取光照输出颜色纹理
	Texture* getOutputColorTexture() const;

	/// 设置IBL辐照度立方体贴图
	void setIrradianceMap(GLuint tex) { mIrradianceMap = tex; }

	/// 设置IBL预滤波环境贴图
	void setPrefilteredMap(GLuint tex) { mPrefilteredMap = tex; }

	/// 设置BRDF积分查找表
	void setBRDFLUT(GLuint tex) { mBRDFLUT = tex; }

private:
	Framebuffer* mGBuffer{ nullptr };  ///< GBuffer引用（不拥有所有权）
	UBOManager*  mUBOManager{ nullptr };

	std::unique_ptr<Framebuffer> mOutputFBO;  ///< 光照输出HDR FBO
	Geometry* mScreenQuad{ nullptr };          ///< 全屏四边形几何

	/// IBL纹理句柄
	GLuint mIrradianceMap{ 0 };
	GLuint mPrefilteredMap{ 0 };
	GLuint mBRDFLUT{ 0 };

	int mWidth{ 0 };
	int mHeight{ 0 };
};
