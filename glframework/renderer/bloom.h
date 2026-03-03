#pragma once
#include "../core.h"
#include "../framebuffer/framebuffer.h"
#include "../geometry.h"

class Shader;

class Bloom {
public:
	Bloom(int width, int height, int minResolution = 32);
	~Bloom();

	void doBloom(Framebuffer* srcFbo);

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
	float mThreshold{ 1.0f };
	float mBloomRadius{ 0.1f };
	float mBloomAttenuation{ 1.0f };
	float mBloomIntensity{ 1.0f };
};