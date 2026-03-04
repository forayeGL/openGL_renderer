#pragma once
#include "../scene.h"
#include "../framebuffer/framebuffer.h"
#include "../../application/camera/camera.h"
#include "../light/directionalLight.h"
#include "../light/pointLight.h"
#include "ubo_manager.h"
#include <vector>
#include <memory>

class Renderer;
class Bloom;

class RenderPipeline {
public:
	RenderPipeline(int width, int height);
	~RenderPipeline();

	void execute(
		Scene* mainScene,
		Scene* postScene,
		Camera* camera,
		DirectionalLight* dirLight,
		const std::vector<PointLight*>& pointLights
	);

	void setClearColor(glm::vec3 color);

	void setRenderMode(RenderMode mode);
	RenderMode getRenderMode() const;

	void setAmbientColor(glm::vec3 c);

	Renderer* getRenderer() const { return mRenderer.get(); }
	Bloom* getBloom() const { return mBloom.get(); }
	Texture* getResolveColorAttachment() const { return mFboResolve->mColorAttachment; }

private:
	std::unique_ptr<Renderer>    mRenderer;
	std::unique_ptr<Bloom>       mBloom;
	std::unique_ptr<Framebuffer> mFboMulti;
	std::unique_ptr<Framebuffer> mFboResolve;
	int mWidth{ 0 };
	int mHeight{ 0 };
};
