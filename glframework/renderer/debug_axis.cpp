#include "debug_axis.h"
#include "../geometry.h"
#include "../shader_manager.h"
#include "../../application/camera/camera.h"

DebugAxis::DebugAxis() {
	std::vector<float> positions = {
		0.0f, 0.0f, 0.0f, 10.0f, 0.0f, 0.0f, // X-axis
		0.0f, 0.0f, 0.0f, 0.0f, 10.0f, 0.0f, // Y-axis
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 10.0f  // Z-axis
	};

	std::vector<float> normals(18, 0.0f);
	std::vector<float> uvs(12, 0.0f);
	
	std::vector<float> colors = {
		1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, // Red for X
		0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, // Green for Y
		0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f  // Blue for Z
	};

	std::vector<unsigned int> indices = { 0, 1, 2, 3, 4, 5 };

	mGeometry = std::make_unique<Geometry>(positions, normals, uvs, colors, indices);
	mShader = ShaderManager::getInstance().getOrCreate("assets/shaders/debug/axis.vert", "assets/shaders/debug/axis.frag");
}

DebugAxis::~DebugAxis() = default;

void DebugAxis::render(const RenderContext& ctx, unsigned int fbo, int width, int height) {
	if (!ctx.camera) return;

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, width, height);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	mShader->begin();
	mShader->setMatrix4x4("viewMatrix", ctx.camera->getViewMatrix());
	mShader->setMatrix4x4("projectionMatrix", ctx.camera->getProjectionMatrix());

	glBindVertexArray(mGeometry->getVao());
	glDrawElements(GL_LINES, mGeometry->getIndicesCount(), GL_UNSIGNED_INT, 0);

	glBindVertexArray(0);
}