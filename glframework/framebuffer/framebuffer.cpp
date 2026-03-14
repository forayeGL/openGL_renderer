#include "framebuffer.h"

Framebuffer* Framebuffer::createShadowFbo(unsigned int width, unsigned int height) {
	Framebuffer* fb = new Framebuffer();
	fb->mWidth = width;
	fb->mHeight = height;

	glGenFramebuffers(1, &fb->mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, fb->mFBO);

	fb->mDepthAttachment = Texture::createDepthAttachment(width, height, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, fb->mDepthAttachment->getTexture(), 0);
	
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Error: Shadow Framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return fb;
}

Framebuffer* Framebuffer::createCSMShadowFbo(unsigned int width, unsigned int height, unsigned int layerNumber) {
	Framebuffer* fb = new Framebuffer();

	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	//加入深度附件
	Texture* depthAttachment = Texture::createDepthAttachmentCSMArray(width, height, layerNumber, 0);
	glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthAttachment->getTexture(), 0, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Error: CSMShadow Framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	fb->mFBO = fbo;
	fb->mDepthAttachment = depthAttachment;
	fb->mWidth = width;
	fb->mHeight = height;

	return fb;
}

Framebuffer* Framebuffer::createPointShadowFbo(unsigned int width, unsigned int height) {
	Framebuffer* fb = new Framebuffer();

	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	Texture* depthAttachment = Texture::createDepthAttachmentCubeMap(width, height, 0);
	glFramebufferTexture2D(
		GL_FRAMEBUFFER,
		GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		depthAttachment->getTexture(),
		0
	);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Error: PointShadow Framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	fb->mFBO = fbo;
	fb->mDepthAttachment = depthAttachment;
	fb->mWidth = width;
	fb->mHeight = height;

	return fb;
}

Framebuffer* Framebuffer::createMultiSampleFbo(unsigned int width, unsigned int height, unsigned int samples) {
	Framebuffer* fb = new Framebuffer();

	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	auto colorAttachment = Texture::createMultiSampleTexture(width, height, samples, GL_RGBA, 0);
	auto dsAttachment = Texture::createMultiSampleTexture(width, height, samples, GL_DEPTH24_STENCIL8, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorAttachment->getTexture(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, dsAttachment->getTexture(), 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	fb->mFBO = fbo;
	fb->mDepthStencilAttachment = dsAttachment;
	fb->mColorAttachment = colorAttachment;
	fb->mWidth = width;
	fb->mHeight = height;

	return fb;
}

Framebuffer* Framebuffer::createHDRFbo(unsigned int width, unsigned int height) {
	Framebuffer* fb = new Framebuffer();

	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	auto colorAttachment = Texture::createHDRTexture(width, height, 0);
	auto dsAttachment = Texture::createDepthStencilAttachment(width, height, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment->getTexture(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, dsAttachment->getTexture(), 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	fb->mFBO = fbo;
	fb->mDepthStencilAttachment = dsAttachment;
	fb->mColorAttachment = colorAttachment;
	fb->mWidth = width;
	fb->mHeight = height;

	return fb;
}

Framebuffer* Framebuffer::createHDRBloomFbo(unsigned int width, unsigned int height) {
	Framebuffer* fb = new Framebuffer();

	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	auto colorAttachment = Texture::createHDRTexture(width, height, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment->getTexture(), 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	fb->mFBO = fbo;
	fb->mColorAttachment = colorAttachment;
	fb->mWidth = width;
	fb->mHeight = height;

	return fb;
}

Framebuffer* Framebuffer::createMultiSampleHDRFbo(unsigned int width, unsigned int height, unsigned int samples) {
	Framebuffer* fb = new Framebuffer();

	unsigned int fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	auto colorAttachment = Texture::createMultiSampleTexture(width, height, samples, GL_RGB16F, 0);
	auto dsAttachment = Texture::createMultiSampleTexture(width, height, samples, GL_DEPTH24_STENCIL8, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, colorAttachment->getTexture(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D_MULTISAMPLE, dsAttachment->getTexture(), 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	fb->mFBO = fbo;
	fb->mDepthStencilAttachment = dsAttachment;
	fb->mColorAttachment = colorAttachment;
	fb->mWidth = width;
	fb->mHeight = height;

	return fb;
}

Framebuffer* Framebuffer::createGBufferFbo(unsigned int width, unsigned int height) {
	/*
	 * GBuffer布局（延迟渲染几何阶段输出）：
	 *   RT0 (GL_RGBA16F): 世界空间位置 (xyz) + 备用 (w)
	 *   RT1 (GL_RGBA16F): 世界空间法线 (xyz) + 备用 (w)
	 *   RT2 (GL_RGBA8):   反照率 (rgb) + 金属度 (a)
	 *   RT3 (GL_RGBA8):   粗糙度 (r) + AO (g) + 自发光标志 (b) + 备用 (a)
	 *   Depth+Stencil:    GL_DEPTH24_STENCIL8
	 */
	Framebuffer* fb = new Framebuffer();
	fb->mWidth = width;
	fb->mHeight = height;

	glGenFramebuffers(1, &fb->mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, fb->mFBO);

	// 每个RT的内部格式
	GLenum internalFormats[] = { GL_RGBA16F, GL_RGBA16F, GL_RGBA8, GL_RGBA8 };
	GLenum dataFormats[]     = { GL_RGBA,    GL_RGBA,    GL_RGBA,  GL_RGBA };
	GLenum dataTypes[]       = { GL_FLOAT,   GL_FLOAT,   GL_UNSIGNED_BYTE, GL_UNSIGNED_BYTE };
	constexpr int numRT = 4;

	for (int i = 0; i < numRT; i++) {
		Texture* attachment = Texture::createGBufferAttachment(
			width, height, internalFormats[i], dataFormats[i], dataTypes[i], i);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i,
			GL_TEXTURE_2D, attachment->getTexture(), 0);
		fb->mColorAttachments.push_back(attachment);
	}

	// 第一个附件也设为主颜色附件（兼容现有接口）
	fb->mColorAttachment = fb->mColorAttachments[0];

	// 设置MRT绘制目标
	GLenum drawBuffers[] = {
		GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1,
		GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3
	};
	glDrawBuffers(numRT, drawBuffers);

	// 深度+模板附件
	fb->mDepthStencilAttachment = Texture::createDepthStencilAttachment(width, height, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,
		fb->mDepthStencilAttachment->getTexture(), 0);

	// 检查FBO完整性
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Error: GBuffer Framebuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return fb;
}

Framebuffer::Framebuffer() {

}

Framebuffer::Framebuffer(unsigned int width, unsigned int height) {
	mWidth = width;
	mHeight = height;

	//1 生成fbo对象并且绑定
	glGenFramebuffers(1, &mFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);


	//2 生成颜色附件，并且加入fbo
	mColorAttachment = Texture::createColorAttachment(mWidth, mHeight, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mColorAttachment->getTexture(), 0);

	//3 生成depth Stencil附件，加入fbo
	mDepthStencilAttachment = Texture::createDepthStencilAttachment(mWidth, mHeight, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, mDepthStencilAttachment->getTexture(), 0);

	//检查当前构建的fbo是否完整
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Error:FrameBuffer is not complete!" << std::endl;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer() {
	if (mFBO) {
		glDeleteFramebuffers(1, &mFBO);
		mFBO = 0;
	}

	// 清理GBuffer额外颜色附件（跳过与mColorAttachment相同的指针）
	for (auto* tex : mColorAttachments) {
		if (tex && tex != mColorAttachment) {
			delete tex;
		}
	}
	mColorAttachments.clear();

	if (mColorAttachment != nullptr) {
		delete mColorAttachment;
		mColorAttachment = nullptr;
	}

	if (mDepthStencilAttachment != nullptr) {
		delete mDepthStencilAttachment;
		mDepthStencilAttachment = nullptr;
	}

	if (mDepthAttachment != nullptr) {
		delete mDepthAttachment;
		mDepthAttachment = nullptr;
	}
}