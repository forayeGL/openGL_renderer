#include "debug_axis_pass.h"
#include "../shader_manager.h"
#include "../../application/camera/camera.h"

DebugAxisPass::~DebugAxisPass() {
	if (mVbo) { glDeleteBuffers(1, &mVbo); }
	if (mVao) { glDeleteVertexArrays(1, &mVao); }
}

void DebugAxisPass::init() {
	mShader = ShaderManager::getInstance().getOrCreate(
		"assets/shaders/debug/axis.vert",
		"assets/shaders/debug/axis.frag"
	);
	buildGeometry();
}

// Each vertex: x, y, z, r, g, b  (6 floats, interleaved)
void DebugAxisPass::buildGeometry() {
	std::vector<float> verts;

	// --- Axes ---
	// X axis (red)
	verts.insert(verts.end(), { 0.0f,           0.0f, 0.0f, 1.0f, 0.0f, 0.0f });
	verts.insert(verts.end(), { mAxisLength,     0.0f, 0.0f, 1.0f, 0.0f, 0.0f });
	// Y axis (green)
	verts.insert(verts.end(), { 0.0f, 0.0f,           0.0f, 0.0f, 1.0f, 0.0f });
	verts.insert(verts.end(), { 0.0f, mAxisLength,     0.0f, 0.0f, 1.0f, 0.0f });
	// Z axis (blue)
	verts.insert(verts.end(), { 0.0f, 0.0f, 0.0f,           0.0f, 0.0f, 1.0f });
	verts.insert(verts.end(), { 0.0f, 0.0f, mAxisLength,     0.0f, 0.0f, 1.0f });

	// --- Grid on the XZ plane (y = 0) ---
	const float extent = mGridHalfSize * mGridStep;
	// Lines parallel to the X axis (vary Z)
	for (int i = -mGridHalfSize; i <= mGridHalfSize; i++) {
		float z = i * mGridStep;
		// Highlight the Z=0 line with a slightly brighter color
		float brightness = (i == 0) ? 0.6f : 0.3f;
		verts.insert(verts.end(), { -extent, 0.0f, z, brightness, brightness, brightness });
		verts.insert(verts.end(), {  extent, 0.0f, z, brightness, brightness, brightness });
	}
	// Lines parallel to the Z axis (vary X)
	for (int i = -mGridHalfSize; i <= mGridHalfSize; i++) {
		float x = i * mGridStep;
		float brightness = (i == 0) ? 0.6f : 0.3f;
		verts.insert(verts.end(), { x, 0.0f, -extent, brightness, brightness, brightness });
		verts.insert(verts.end(), { x, 0.0f,  extent, brightness, brightness, brightness });
	}

	mVertexCount = static_cast<int>(verts.size()) / 6;

	glGenVertexArrays(1, &mVao);
	glGenBuffers(1, &mVbo);

	glBindVertexArray(mVao);
	glBindBuffer(GL_ARRAY_BUFFER, mVbo);
	glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

	constexpr GLsizei stride = 6 * sizeof(float);
	// location 0: position (vec3)
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
	// location 1: color (vec3)
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));

	glBindVertexArray(0);
}

void DebugAxisPass::execute(Camera* camera, unsigned int fbo, int viewportWidth, int viewportHeight) {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, viewportWidth, viewportHeight);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_FALSE);   // read depth (axes occluded by closer geometry) but do not write it
	glDisable(GL_BLEND);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	mShader->begin();
	mShader->setMatrix4x4("viewMatrix",       camera->getViewMatrix());
	mShader->setMatrix4x4("projectionMatrix", camera->getProjectionMatrix());

	glBindVertexArray(mVao);
	glDrawArrays(GL_LINES, 0, mVertexCount);
	glBindVertexArray(0);

	mShader->end();

	// Restore depth-mask so subsequent passes can write depth normally
	glDepthMask(GL_TRUE);
}
