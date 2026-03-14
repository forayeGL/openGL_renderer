#pragma once
#include "render_context.h"
#include <memory>
#include <vector>

class Geometry;
class Shader;

class DebugAxis {
public:
	DebugAxis();
	~DebugAxis();

	void render(const RenderContext& ctx, unsigned int fbo, int width, int height);

private:
	std::unique_ptr<Geometry> mGeometry;
	Shader* mShader{ nullptr };
};