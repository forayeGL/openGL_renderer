#pragma once
#include "../core.h"

class Camera;
class Shader;

// Renders a Cartesian coordinate system (X/Y/Z axes and a ground grid) for
// debugging world-space coordinates. Designed to be shared across pipelines:
// construct once, call init() after the GL context is ready, then call
// execute() each frame from any IRenderPipeline implementation.
class DebugAxisPass {
public:
	DebugAxisPass() = default;
	~DebugAxisPass();

	// Allocates GPU resources (VAO, VBO, shader). Must be called once after
	// the OpenGL context is available.
	void init();

	// Draws axes and grid into fbo using the given camera and viewport size.
	void execute(Camera* camera, unsigned int fbo, int viewportWidth, int viewportHeight);

	// Axis length in world units (default 10).
	float mAxisLength{ 10.0f };

	// Grid extends from -mGridHalfSize to +mGridHalfSize on both X and Z
	// axes. Each cell is mGridStep world units wide (default step = 1).
	int   mGridHalfSize{ 10 };
	float mGridStep{ 1.0f };

private:
	void buildGeometry();

	GLuint mVao{ 0 };
	GLuint mVbo{ 0 };
	int    mVertexCount{ 0 };

	Shader* mShader{ nullptr };
};
