#include "renderer.h"
#include <iostream>

#include "../mesh/instancedMesh.h"
#include "../../application/camera/orthographicCamera.h"
#include "../shader_manager.h"
#include "../light/shadow/directionalLightShadow.h"
#include "../light/shadow/pointLightShadow.h"

#include <string>
#include <algorithm>


Renderer::Renderer() {
	mUBOManager.init();
}

Renderer::~Renderer() {
}

void Renderer::setClearColor(glm::vec3 color) {
	glClearColor(color.r, color.g, color.b, 1.0);
}

void Renderer::msaaResolve(Framebuffer* src, Framebuffer* dst) {
	glBindFramebuffer(GL_READ_FRAMEBUFFER, src->mFBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dst->mFBO);
	glBlitFramebuffer(0, 0, src->mWidth, src->mHeight, 0, 0, dst->mWidth, dst->mHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
}


void Renderer::render(
	Scene* scene,
	Camera* camera,
	DirectionalLight* dirLight,
	const std::vector<PointLight*>& pointLights,
	unsigned int fbo
) {
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);

	glDisable(GL_POLYGON_OFFSET_FILL);
	glDisable(GL_POLYGON_OFFSET_LINE);

	glEnable(GL_STENCIL_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilMask(0xFF);

	glDisable(GL_BLEND);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	mOpacityObjects.clear();
	mTransparentObjects.clear();

	projectObject(scene);

	std::sort(
		mTransparentObjects.begin(),
		mTransparentObjects.end(),
		[camera](const Mesh* a, const Mesh* b) {
			auto viewMatrix = camera->getViewMatrix();
			auto worldPositionA = a->getModelMatrix() * glm::vec4(0.0, 0.0, 0.0, 1.0);
			auto cameraPositionA = viewMatrix * worldPositionA;
			auto worldPositionB = b->getModelMatrix() * glm::vec4(0.0, 0.0, 0.0, 1.0);
			auto cameraPositionB = viewMatrix * worldPositionB;
			return cameraPositionA.z < cameraPositionB.z;
		}
	);

	// Save viewport before shadow passes
	GLint savedViewport[4];
	glGetIntegerv(GL_VIEWPORT, savedViewport);

	// Shadow map passes
	if (dirLight && dirLight->mShadow) {
		renderDirectionalShadow(dirLight, mOpacityObjects);
	}

	for (auto* pl : pointLights) {
		if (pl->mShadow) {
			renderPointShadow(pl, mOpacityObjects);
		}
	}

	// Restore FBO and viewport
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(savedViewport[0], savedViewport[1], savedViewport[2], savedViewport[3]);

	// Update UBOs once per frame
	mUBOManager.updateLights(dirLight, pointLights);
	mUBOManager.updateShadow(dirLight, pointLights);
	mUBOManager.updateRenderSettings(camera, mRenderMode, mShadowType, mAmbientColor);

	// Apply render mode
	if (mRenderMode == RenderMode::Wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	} else {
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Bind shadow textures globally
	if (dirLight && dirLight->mShadow) {
		auto* shadow = dirLight->mShadow;
		if (shadow->mRenderTarget && shadow->mRenderTarget->mDepthAttachment) {
			shadow->mRenderTarget->mDepthAttachment->setUnit(5);
			shadow->mRenderTarget->mDepthAttachment->bind();
		}
	}

	for (int i = 0; i < (int)pointLights.size() && i < MAX_POINT_SHADOW; i++) {
		auto* pl = pointLights[i];
		if (pl->mShadow && pl->mShadow->mRenderTarget && pl->mShadow->mRenderTarget->mDepthAttachment) {
			int unit = 6 + i;
			pl->mShadow->mRenderTarget->mDepthAttachment->setUnit(unit);
			pl->mShadow->mRenderTarget->mDepthAttachment->bind();
		}
	}

	for (int i = 0; i < (int)mOpacityObjects.size(); i++) {
		renderObject(mOpacityObjects[i], camera, dirLight, pointLights);
	}

	for (int i = 0; i < (int)mTransparentObjects.size(); i++) {
		renderObject(mTransparentObjects[i], camera, dirLight, pointLights);
	}

	// Reset polygon mode
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::projectObject(Object* obj) {
	if (obj->getType() == ObjectType::Mesh || obj->getType() == ObjectType::InstancedMesh) {
		Mesh* mesh = (Mesh*)obj;
		auto material = mesh->mMaterial;
		if (material->mBlend) {
			mTransparentObjects.push_back(mesh);
		}
		else {
			mOpacityObjects.push_back(mesh);
		}
	}

	auto& children = obj->getChildren();
	for (int i = 0; i < children.size(); i++) {
		projectObject(children[i]);
	}
}

void Renderer::renderObject(
	Object* object,
	Camera* camera,
	DirectionalLight* dirLight,
	const std::vector<PointLight*>& pointLights
) {
	if (object->getType() == ObjectType::Mesh || object->getType() == ObjectType::InstancedMesh) {
		auto mesh = (Mesh*)object;
		auto geometry = mesh->mGeometry;

		Material* material = nullptr;
		if (mGlobalMaterial != nullptr) {
			material = mGlobalMaterial;
		}
		else {
			material = mesh->mMaterial;
		}

		setDepthState(material);
		setPolygonOffsetState(material);
		setStencilState(material);
		setBlendState(material);
		setFaceCullingState(material);

		Shader* shader = ShaderManager::getInstance().getOrCreate(
			material->getVertexShaderPath(),
			material->getFragmentShaderPath()
		);

		shader->begin();

		// Bind UBOs to this shader program
		mUBOManager.bindToShader(shader->getProgram());

		// Per-object uniforms
		material->applyUniforms(shader, mesh, camera, pointLights);

		// Shadow texture samplers
		// 始终将所有shadow sampler指向非冲突的纹理单元，
		// 防止samplerCube默认指向unit 0导致类型冲突
		shader->setInt("shadowMapSampler", 5);
		for (int i = 0; i < MAX_POINT_SHADOW; i++) {
			shader->setInt("pointShadowMaps[" + std::to_string(i) + "]", 6 + i);
		}

		glBindVertexArray(geometry->getVao());

		if (object->getType() == ObjectType::InstancedMesh) {
			InstancedMesh* im = (InstancedMesh*)mesh;
			// 绑定具体实例的VBO并重置顶点属性指针，以防共享Geometry的VAO导致所有实例都使用最后一个VBO的数据
			im->beforeDraw();
			glDrawElementsInstanced(GL_TRIANGLES, geometry->getIndicesCount(), GL_UNSIGNED_INT, 0, im->mInstanceCount);
		}
		else {
			glDrawElements(GL_TRIANGLES, geometry->getIndicesCount(), GL_UNSIGNED_INT, 0);
		}
	}
}

void Renderer::setDepthState(Material* material) {
	if (material->mDepthTest) {
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(material->mDepthFunc);
	}
	else {
		glDisable(GL_DEPTH_TEST);
	}

	if (material->mDepthWrite) {
		glDepthMask(GL_TRUE);
	}
	else {
		glDepthMask(GL_FALSE);
	}
}
void Renderer::setPolygonOffsetState(Material* material) {
	if (material->mPolygonOffset) {
		glEnable(material->mPolygonOffsetType);
		glPolygonOffset(material->mFactor, material->mUnit);
	}
	else {
		glDisable(GL_POLYGON_OFFSET_FILL);
		glDisable(GL_POLYGON_OFFSET_LINE);
	}
}

void Renderer::setStencilState(Material* material) {
	if (material->mStencilTest) {
		glEnable(GL_STENCIL_TEST);

		glStencilOp(material->mSFail, material->mZFail, material->mZPass);
		glStencilMask(material->mStencilMask);
		glStencilFunc(material->mStencilFunc, material->mStencilRef, material->mStencilFuncMask);

	}
	else {
		glDisable(GL_STENCIL_TEST);
	}
}

void Renderer::setBlendState(Material* material) {
	if (material->mBlend) {
		glEnable(GL_BLEND);
		glBlendFunc(material->mSFactor, material->mDFactor);
	}
	else {
		glDisable(GL_BLEND);
	}
}

void Renderer::setFaceCullingState(Material* material) {
	if (material->mFaceCulling) {
		glEnable(GL_CULL_FACE);
		glFrontFace(material->mFrontFace);
		glCullFace(material->mCullFace);
	}
	else {
		glDisable(GL_CULL_FACE);
	}
}

void Renderer::renderDirectionalShadow(
	DirectionalLight* dirLight,
	const std::vector<Mesh*>& objects
) {
	auto* shadow = static_cast<DirectionalLightShadow*>(dirLight->mShadow);
	auto* fbo = shadow->mRenderTarget;

	glBindFramebuffer(GL_FRAMEBUFFER, fbo->mFBO);
	glViewport(0, 0, fbo->mWidth, fbo->mHeight);
	glClear(GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	auto lightMatrix = shadow->getLightMatrix(dirLight->getModelMatrix());

	Shader* shaderReg = ShaderManager::getInstance().getOrCreate(
		"assets/shaders/shadow/shadow.vert",
		"assets/shaders/shadow/shadow.frag"
	);
	Shader* shaderInst = ShaderManager::getInstance().getOrCreate(
		"assets/shaders/shadow/shadow_instanced.vert",
		"assets/shaders/shadow/shadow.frag"
	);

	for (auto* mesh : objects) {
		bool isInstanced = (mesh->getType() == ObjectType::InstancedMesh);
		Shader* currentShader = isInstanced ? shaderInst : shaderReg;

		currentShader->begin();
		currentShader->setMatrix4x4("lightMatrix", lightMatrix);

		glBindVertexArray(mesh->mGeometry->getVao());
		if (isInstanced) {
			auto* im = static_cast<InstancedMesh*>(mesh);
			im->beforeDraw();
			glDrawElementsInstanced(GL_TRIANGLES, im->mGeometry->getIndicesCount(), GL_UNSIGNED_INT, 0, im->mInstanceCount);
		} else {
			currentShader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
			glDrawElements(GL_TRIANGLES, mesh->mGeometry->getIndicesCount(), GL_UNSIGNED_INT, 0);
		}
	}
}

void Renderer::renderPointShadow(
	PointLight* pointLight,
	const std::vector<Mesh*>& objects
) {
	auto* shadow = static_cast<PointLightShadow*>(pointLight->mShadow);
	auto* fbo = shadow->mRenderTarget;

	auto lightMatrices = shadow->getLightMatrices(pointLight->getPosition());

	Shader* shaderReg = ShaderManager::getInstance().getOrCreate(
		"assets/shaders/shadow/shadowDistance.vert",
		"assets/shaders/shadow/shadowDistance.frag"
	);
	Shader* shaderInst = ShaderManager::getInstance().getOrCreate(
		"assets/shaders/shadow/shadowDistance_instanced.vert",
		"assets/shaders/shadow/shadowDistance.frag"
	);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);

	for (int i = 0; i < 6; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo->mFBO);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER,
			GL_DEPTH_ATTACHMENT,
			GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
			fbo->mDepthAttachment->getTexture(),
			0
		);

		glViewport(0, 0, fbo->mWidth, fbo->mHeight);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		for (auto* mesh : objects) {
			bool isInstanced = (mesh->getType() == ObjectType::InstancedMesh);
			Shader* currentShader = isInstanced ? shaderInst : shaderReg;

			currentShader->begin();
			currentShader->setMatrix4x4("lightMatrix", lightMatrices[i]);
			currentShader->setFloat("far", shadow->mCamera->mFar);
			currentShader->setVector3("lightPosition", pointLight->getPosition());

			glBindVertexArray(mesh->mGeometry->getVao());
			if (isInstanced) {
				auto* im = static_cast<InstancedMesh*>(mesh);
				im->beforeDraw();
				glDrawElementsInstanced(GL_TRIANGLES, im->mGeometry->getIndicesCount(), GL_UNSIGNED_INT, 0, im->mInstanceCount);
			} else {
				currentShader->setMatrix4x4("modelMatrix", mesh->getModelMatrix());
				glDrawElements(GL_TRIANGLES, mesh->mGeometry->getIndicesCount(), GL_UNSIGNED_INT, 0);
			}
		}
	}
}

void Renderer::renderIBLDiffuse(Texture* hdrTex, Framebuffer* fbo)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->mFBO);
	glViewport(0, 0, fbo->mWidth, fbo->mHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glDisable(GL_BLEND);
	Shader* shader = ShaderManager::getInstance().getOrCreate(
		"assets/shaders/advanced/IBL/iblDiffusePass.vert",
		"assets/shaders/advanced/IBL/iblDiffusePass.frag"
	);
	shader->begin();
	shader->setInt("hdrTex", 0);
	hdrTex->setUnit(0);
	hdrTex->bind();
	glBindVertexArray(Geometry::createScreenPlane()->getVao());
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

