#pragma once

#include "../core.h"
#include "../texture.h"
#include <vector>

/**
 * @brief 帧缓冲对象封装
 * 
 * 封装OpenGL FBO及其颜色/深度/模板附件。
 * 提供多种静态工厂方法创建不同用途的FBO：
 * - 阴影贴图FBO（仅深度附件）
 * - HDR渲染FBO（16位浮点颜色）
 * - MSAA抗锯齿FBO
 * - GBuffer FBO（多渲染目标MRT）
 */
class Framebuffer {
public:
	static Framebuffer* createShadowFbo(unsigned int width, unsigned int height);
	static Framebuffer* createCSMShadowFbo(unsigned int width, unsigned int height, unsigned int layerNumber);
	static Framebuffer* createPointShadowFbo(unsigned int width, unsigned int height);
	static Framebuffer* createMultiSampleFbo(unsigned int width, unsigned int height, unsigned int samples);
	static Framebuffer* createHDRFbo(unsigned int width, unsigned int height);
	static Framebuffer* createHDRBloomFbo(unsigned int width, unsigned int height);
	static Framebuffer* createMultiSampleHDRFbo(unsigned int width, unsigned int height, unsigned int samples = 4);

	/**
	 * @brief 创建延迟渲染GBuffer FBO
	 * 
	 * 包含4个颜色附件（MRT）：
	 *   0: 世界空间位置（RGB16F）
	 *   1: 世界空间法线（RGB16F）
	 *   2: 反照率 + 金属度（RGBA8）
	 *   3: 粗糙度 + AO + 自发光标志（RGBA8）
	 * 以及一个深度+模板附件
	 */
	static Framebuffer* createGBufferFbo(unsigned int width, unsigned int height);

	Framebuffer();
	Framebuffer(unsigned int width, unsigned int height);
	~Framebuffer();

public:
	unsigned int mWidth{ 0 };
	unsigned int mHeight{ 0 };

	unsigned int mFBO{ 0 };
	Texture* mColorAttachment{ nullptr };           ///< 主颜色附件（前向渲染/后处理使用）
	Texture* mDepthStencilAttachment{ nullptr };
	Texture* mDepthAttachment{ nullptr };

	/// GBuffer额外颜色附件（MRT），延迟渲染使用
	std::vector<Texture*> mColorAttachments;
};