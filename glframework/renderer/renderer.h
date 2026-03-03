#pragma once
#include <vector>
#include "../core.h"
#include "../mesh/mesh.h"
#include "../../application/camera/camera.h"
#include "../light/directionalLight.h"
#include "../light/pointLight.h"
#include "../light/spotLight.h"
#include "../light/ambientLight.h"
#include "../shader.h"
#include "../scene.h"
#include "../framebuffer/framebuffer.h"

class Renderer {
public:
	Renderer();
	~Renderer();

	void render(
		Scene* scene,
		Camera* camera,
		const std::vector<PointLight*>& pointLights,
		unsigned int fbo = 0
	);

	void renderObject(
		Object* object,
		Camera* camera,
		const std::vector<PointLight*>& pointLights
	);

	void setClearColor(glm::vec3 color);

	void msaaResolve(Framebuffer* src, Framebuffer* dst);

public:
	Material* mGlobalMaterial{ nullptr };

private:
	void projectObject(Object* obj);

	void setDepthState(Material* material);
	void setPolygonOffsetState(Material* material);
	void setStencilState(Material* material);
	void setBlendState(Material* material);
	void setFaceCullingState(Material* material);

private:
	std::vector<Mesh*>	mOpacityObjects{};
	std::vector<Mesh*>	mTransparentObjects{};
};