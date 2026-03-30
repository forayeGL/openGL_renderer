#pragma once
#include "../core.h"
#include "../framebuffer/framebuffer.h"
#include "../geometry.h"

class Shader;

class Bloom {
public:
	Bloom(int width, int height, int minResolution = 32);
	~Bloom();

  /**
	 * @brief 对目标FBO执行Bloom，并将结果写回目标FBO
	 * @param targetFbo Bloom合成输出目标
	 * @param sourceFbo 原图来源（可选）
	 *
	 * 当sourceFbo为空时，会先将targetFbo备份到内部origin FBO，
	 * 然后执行亮度提取/上下采样并合成回targetFbo。
	 * 当sourceFbo非空时，直接以sourceFbo作为原图来源，
	 * 避免额外备份拷贝。
	 */
	void doBloom(Framebuffer* targetFbo, Framebuffer* sourceFbo = nullptr);

private:
	void extractBright(Framebuffer* src, Framebuffer* dst);
	void downSample(Framebuffer* src, Framebuffer* dst);
	void upSample(Framebuffer* target, Framebuffer* lowerResFbo, Framebuffer* higherResFbo);
	void merge(Framebuffer* target, Framebuffer* origin, Framebuffer* bloom);

private:
	std::vector<Framebuffer*> mDownSamples{};
	std::vector<Framebuffer*> mUpSamples{};
	int mWidth{ 0 };
	int mHeight{ 0 };
	int mMipLevels{ 0 };

	Geometry* mQuad{ nullptr };
	Framebuffer* mOriginFbo{ nullptr };

	Shader* mExtractBrightShader{ nullptr };
	Shader* mUpSampleShader{ nullptr };
	Shader* mMergeShader{ nullptr };

public:
	float mThreshold{ 1.2f };
	float mBloomRadius{ 0.1f };
	float mBloomAttenuation{ 0.8f };
	float mBloomIntensity{ 0.3f };
};