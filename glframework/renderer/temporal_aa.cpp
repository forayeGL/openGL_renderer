#include "temporal_aa.h"

#include "../shader.h"
#include "../shader_manager.h"

#include <algorithm>

TemporalAA::TemporalAA(int width, int height)
	: mWidth(width)
	, mHeight(height) {
	mHistoryA = std::unique_ptr<Framebuffer>(Framebuffer::createHDRBloomFbo(width, height));
	mHistoryB = std::unique_ptr<Framebuffer>(Framebuffer::createHDRBloomFbo(width, height));
	mQuad = Geometry::createScreenPlane();

	auto& sm = ShaderManager::getInstance();
	mResolveShader = sm.getOrCreate(
		"assets/shaders/advanced/taa/taa_resolve.vert",
		"assets/shaders/advanced/taa/taa_resolve.frag"
	);
}

TemporalAA::~TemporalAA() = default;

void TemporalAA::resetHistory() {
	mHasHistory = false;
}

float TemporalAA::halton(int index, int base) {
	float f = 1.0f;
	float r = 0.0f;
	int i = index;
	while (i > 0) {
		f = f / static_cast<float>(base);
		r = r + f * static_cast<float>(i % base);
		i = i / base;
	}
	return r;
}

glm::vec2 TemporalAA::consumeJitterNdc() {
	const int sampleIdx = static_cast<int>(mFrameIndex % 8u) + 1;
	mFrameIndex++;

	const float jitterX = halton(sampleIdx, 2) - 0.5f;
	const float jitterY = halton(sampleIdx, 3) - 0.5f;

	return glm::vec2(
		(2.0f * jitterX) / std::max(static_cast<float>(mWidth), 1.0f),
		(2.0f * jitterY) / std::max(static_cast<float>(mHeight), 1.0f)
	);
}

void TemporalAA::blitColor(Framebuffer* src, Framebuffer* dst) {
	if (!src || !dst) return;
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->mFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->mFBO);
	glBlitFramebuffer(
		0, 0, src->mWidth, src->mHeight,
		0, 0, dst->mWidth, dst->mHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR
	);
}

void TemporalAA::resolve(
	Framebuffer* currentSource,
	Framebuffer* outputTarget,
	float historyBlend,
	bool neighborhoodClamp,
	bool resetHistory
) {
	if (!currentSource || !outputTarget || !mHistoryA || !mHistoryB || !mResolveShader || !mQuad) {
		return;
	}

	if (resetHistory) {
		mHasHistory = false;
	}

	const int historyWriteIndex = 1 - mHistoryReadIndex;
	Framebuffer* historyRead = (mHistoryReadIndex == 0) ? mHistoryA.get() : mHistoryB.get();
	Framebuffer* historyWrite = (historyWriteIndex == 0) ? mHistoryA.get() : mHistoryB.get();

	GLint prevFbo = 0;
	GLint prevViewport[4]{};
	glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
	glGetIntegerv(GL_VIEWPORT, prevViewport);

	const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
	const GLboolean blendEnabled = glIsEnabled(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	if (!mHasHistory) {
		blitColor(currentSource, outputTarget);
	} else {
		glBindFramebuffer(GL_FRAMEBUFFER, outputTarget->mFBO);
		glViewport(0, 0, outputTarget->mWidth, outputTarget->mHeight);
		glClear(GL_COLOR_BUFFER_BIT);

		mResolveShader->begin();
		currentSource->mColorAttachment->setUnit(0);
		currentSource->mColorAttachment->bind();
		mResolveShader->setInt("currentTex", 0);

		historyRead->mColorAttachment->setUnit(1);
		historyRead->mColorAttachment->bind();
		mResolveShader->setInt("historyTex", 1);

		const float clampedBlend = std::clamp(historyBlend, 0.0f, 0.98f);
		mResolveShader->setFloat("historyBlend", clampedBlend);
		mResolveShader->setInt("neighborhoodClamp", neighborhoodClamp ? 1 : 0);
		mResolveShader->setFloat("texelSizeX", 1.0f / std::max(static_cast<float>(mWidth), 1.0f));
		mResolveShader->setFloat("texelSizeY", 1.0f / std::max(static_cast<float>(mHeight), 1.0f));

		glBindVertexArray(mQuad->getVao());
		glDrawElements(GL_TRIANGLES, mQuad->getIndicesCount(), GL_UNSIGNED_INT, 0);
		mResolveShader->end();
	}

	blitColor(outputTarget, historyWrite);
	mHistoryReadIndex = historyWriteIndex;
	mHasHistory = true;

	if (depthEnabled) glEnable(GL_DEPTH_TEST);
	if (blendEnabled) glEnable(GL_BLEND);

	glBindFramebuffer(GL_FRAMEBUFFER, prevFbo);
	glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
}
