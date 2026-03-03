#pragma once
#include "../scene.h"
#include "../framebuffer/framebuffer.h"
#include "../../application/camera/camera.h"
#include "../light/pointLight.h"
#include <vector>

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
		const std::vector<PointLight*>& pointLights
	);

	void setClearColor(glm::vec3 color);

	Renderer* getRenderer() const { return mRenderer; }
	Bloom* getBloom() const { return mBloom; }
	Texture* getResolveColorAttachment() const { return mFboResolve->mColorAttachment; }

private:
	Renderer*    mRenderer{ nullptr };
	Bloom*       mBloom{ nullptr };
	Framebuffer* mFboMulti{ nullptr };
	Framebuffer* mFboResolve{ nullptr };
};
