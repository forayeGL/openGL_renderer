#pragma once

#include "../core.h"
#include "../framebuffer/framebuffer.h"
#include "../geometry.h"
#include <memory>

class Shader;

class TemporalAA {
public:
	TemporalAA(int width, int height);
	~TemporalAA();

	void resetHistory();

	glm::vec2 consumeJitterNdc();

	void resolve(
		Framebuffer* currentSource,
		Framebuffer* outputTarget,
		float historyBlend,
		bool neighborhoodClamp,
		bool resetHistory
	);

private:
	static float halton(int index, int base);
	void blitColor(Framebuffer* src, Framebuffer* dst);

private:
	int mWidth{ 0 };
	int mHeight{ 0 };
	unsigned int mFrameIndex{ 0 };
	bool mHasHistory{ false };
	int mHistoryReadIndex{ 0 };

	std::unique_ptr<Framebuffer> mHistoryA;
	std::unique_ptr<Framebuffer> mHistoryB;

	Geometry* mQuad{ nullptr };
	Shader* mResolveShader{ nullptr };
};
