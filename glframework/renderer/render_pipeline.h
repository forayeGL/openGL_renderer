#pragma once
// Backward-compatibility shim: RenderPipeline is now DefaultRenderPipeline.
// New code should include default_render_pipeline.h or i_render_pipeline.h directly.
#include "default_render_pipeline.h"
using RenderPipeline = DefaultRenderPipeline;
