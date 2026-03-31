#include "postprocess_pass.h"
#include "../bloom.h"
#include "../temporal_aa.h"

PostProcessPass::PostProcessPass() = default;
PostProcessPass::~PostProcessPass() = default;

void PostProcessPass::init(int width, int height) {
	mWidth = width;
	mHeight = height;

	// 创建Bloom处理器
	mBloom = std::make_unique<Bloom>(width, height);
	mTAA = std::make_unique<TemporalAA>(width, height);

	// 创建后处理输出FBO（与光照FBO同等规格的HDR FBO）
	mOutputFBO = std::unique_ptr<Framebuffer>(Framebuffer::createHDRFbo(width, height));
}

Texture* PostProcessPass::getOutputColorTexture() const {
	return mOutputFBO ? mOutputFBO->mColorAttachment : nullptr;
}

void PostProcessPass::execute(const RenderContext& ctx) {
	if (!mInputFBO || !mOutputFBO) return;

 if (ctx.enableTAA && mTAA) {
		mTAA->resolve(
			mInputFBO,
			mOutputFBO.get(),
			ctx.taaBlendFactor,
			ctx.taaNeighborhoodClamp,
			ctx.taaResetHistory
		);

		// TAA输出已经在mOutputFBO，Bloom直接原地处理
		mBloom->doBloom(mOutputFBO.get());
	} else {
		if (mTAA) {
			mTAA->resetHistory();
		}

		// 无TAA路径保持优化：直接使用输入作为Bloom原图，避免额外备份
		mBloom->doBloom(mOutputFBO.get(), mInputFBO);
	}
}
