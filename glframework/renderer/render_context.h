#pragma once
#include "../scene.h"
#include "../../application/camera/camera.h"
#include "../light/directionalLight.h"
#include "../light/pointLight.h"
#include <vector>

// RenderContext bundles all per-frame scene data into a single token.
// Passed into IRenderPipeline::execute() so that pipelines are decoupled
// from individual scene management concerns.
struct RenderContext {
	Scene*                       mainScene{ nullptr };
	Scene*                       postScene{ nullptr };
	Camera*                      camera{ nullptr };
	DirectionalLight*            dirLight{ nullptr };
	std::vector<PointLight*>*    pointLights{ nullptr };

	// Pipeline-level settings (written by GUI, consumed by pipeline)
	glm::vec3 clearColor{};
	glm::vec3 ambientColor{ 0.15f };
	int       renderModeIdx{ 0 };
	bool      showDebugAxis{ false };
};
