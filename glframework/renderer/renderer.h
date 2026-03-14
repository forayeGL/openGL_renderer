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
#include "ubo_manager.h"

class Renderer {
public:
	Renderer();
	~Renderer();

	void render(
		Scene* scene,
		Camera* camera,
		DirectionalLight* dirLight,
		const std::vector<PointLight*>& pointLights,
		unsigned int fbo = 0
	);

	void renderObject(
		Object* object,
		Camera* camera,
		DirectionalLight* dirLight,
		const std::vector<PointLight*>& pointLights
	);

	void renderDirectionalShadow(
		DirectionalLight* dirLight,
		const std::vector<Mesh*>& objects
	);

	void renderPointShadow(
		PointLight* pointLight,
		const std::vector<Mesh*>& objects
	);

	void renderIBLDiffuse(Texture* hdrTex, Framebuffer* fbo);

	void setClearColor(glm::vec3 color);

	void msaaResolve(Framebuffer* src, Framebuffer* dst);

	UBOManager& getUBOManager() { return mUBOManager; }

	void setRenderMode(RenderMode mode) { mRenderMode = mode; }
	RenderMode getRenderMode() const { return mRenderMode; }

	void setShadowType(int type) { mShadowType = type; }
	int getShadowType() const { return mShadowType; }

	void setAmbientColor(glm::vec3 c) { mAmbientColor = c; }

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
	std::vector<Mesh*> mOpacityObjects{};
	std::vector<Mesh*> mTransparentObjects{};

	UBOManager  mUBOManager;
	RenderMode  mRenderMode{ RenderMode::Fill };
	int         mShadowType{ 1 };
	glm::vec3   mAmbientColor{ 0.15f };
};