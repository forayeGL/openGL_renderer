#pragma once
#include "i_render_pipeline.h"
#include "../framebuffer/framebuffer.h"
#include "ubo_manager.h"
#include <memory>

class Renderer;
class Bloom;
class DebugAxis;
class TemporalAA;

// Default forward+ pipeline: shadow passes → MSAA HDR scene → bloom → post screen.
// This is the concrete replacement for the old RenderPipeline class.
class DefaultRenderPipeline : public IRenderPipeline {
public:
	DefaultRenderPipeline();
	~DefaultRenderPipeline() override; // defined in .cpp where Renderer/Bloom are complete

	void init(int width, int height) override;
    void resize(int width, int height) override;
	void execute(const RenderContext& ctx) override;
	Texture* getResolveColorAttachment() const override;

	// Fine-grained accessors used by GUI panels
	Renderer* getRenderer() const { return mRenderer.get(); }
	Bloom*    getBloom()    const { return mBloom.get(); }
	TemporalAA* getTAA() const { return mTAA.get(); }

private:
	std::unique_ptr<Renderer>    mRenderer;
	std::unique_ptr<Bloom>       mBloom;
 std::unique_ptr<TemporalAA>  mTAA;
	std::unique_ptr<Framebuffer> mFboMulti;
   std::unique_ptr<Framebuffer> mFboPreTAA;
	std::unique_ptr<Framebuffer> mFboResolve;
	std::unique_ptr<DebugAxis>   mDebugAxis;
	UBOManager mUBOManager;
    bool mHasPrevCameraState{ false };
	glm::vec3 mPrevCameraPosition{ 0.0f };
	glm::vec3 mPrevCameraUp{ 0.0f, 1.0f, 0.0f };
	glm::vec3 mPrevCameraRight{ 1.0f, 0.0f, 0.0f };
	int mWidth{ 0 };
	int mHeight{ 0 };
};
