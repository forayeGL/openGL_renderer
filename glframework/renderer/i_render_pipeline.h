#pragma once
#include "render_context.h"
#include "../texture.h"

// Abstract base for all render pipelines.
// Implement this to create custom pipelines (deferred, toon, etc.)
// and plug them into RendererApp without touching application code.
class IRenderPipeline {
public:
	virtual ~IRenderPipeline() = default;

	// Called once after OpenGL context is ready.
	virtual void init(int width, int height) = 0;

	// Execute one full frame using the data in ctx.
	virtual void execute(const RenderContext& ctx) = 0;

	// Returns the resolved (non-MSAA) HDR color attachment for post-process sampling.
	virtual Texture* getResolveColorAttachment() const = 0;
};
