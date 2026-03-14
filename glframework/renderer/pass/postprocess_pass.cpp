#include "postprocess_pass.h"
#include "../bloom.h"

PostProcessPass::PostProcessPass() = default;
PostProcessPass::~PostProcessPass() = default;

void PostProcessPass::init(int width, int height) {
	mWidth = width;
	mHeight = height;

	// 创建Bloom处理器
	mBloom = std::make_unique<Bloom>(width, height);

	// 创建后处理输出FBO（与光照FBO同等规格的HDR FBO）
	mOutputFBO = std::unique_ptr<Framebuffer>(Framebuffer::createHDRFbo(width, height));
}

Texture* PostProcessPass::getOutputColorTexture() const {
	return mOutputFBO ? mOutputFBO->mColorAttachment : nullptr;
}

void PostProcessPass::execute(const RenderContext& /*ctx*/) {
	if (!mInputFBO || !mOutputFBO) return;

	// 先将输入FBO的内容复制到输出FBO（保留原始图像）
	glBindFramebuffer(GL_READ_FRAMEBUFFER, mInputFBO->mFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mOutputFBO->mFBO);
	glBlitFramebuffer(
		0, 0, mInputFBO->mWidth, mInputFBO->mHeight,
		0, 0, mOutputFBO->mWidth, mOutputFBO->mHeight,
		GL_COLOR_BUFFER_BIT, GL_LINEAR
	);

	// 对输出FBO执行Bloom后处理
	mBloom->doBloom(mOutputFBO.get());
}
