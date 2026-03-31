#pragma once
#include "../material.h"
#include "../../core.h"

class Texture;

/**
 * @brief PBR物理材质
 * 
 * 基于金属度-粗糙度工作流的PBR材质，支持：
 * - 反照率（Albedo）贴图
 * - 法线（Normal）贴图
 * - 金属度（Metallic）贴图
 * - 粗糙度（Roughness）贴图
 * - 环境光遮蔽（AO）贴图
 * - IBL辐照度贴图（间接漫反射）
 * - IBL预滤波环境贴图（间接镜面反射）
 * - BRDF积分查找表
 * 
 * 在延迟渲染管线中作为GBuffer填充阶段的主材质使用。
 * 在前向渲染管线中直接计算PBR光照。
 */
class PbrMaterial : public Material {
public:
	PbrMaterial();
	~PbrMaterial();

	const char* getVertexShaderPath() const override;
	const char* getFragmentShaderPath() const override;
	void applyUniforms(
		Shader* shader,
		Mesh* mesh,
		Camera* camera,
		const std::vector<PointLight*>& pointLights
	) override;

public:
	// 基础PBR纹理
	Texture* mAlbedo{ nullptr };              ///< 反照率贴图
	Texture* mNormal{ nullptr };              ///< 法线贴图
	Texture* mMetallic{ nullptr };            ///< 金属度贴图
	Texture* mRoughness{ nullptr };           ///< 粗糙度贴图
	Texture* mAO{ nullptr };                  ///< 环境光遮蔽贴图

	// IBL资源（GLuint句柄，由IBLGenerator生成）
	GLuint mIrradianceIndirect{ 0 };  ///< IBL漫反射辐照度贴图
	GLuint mPrefilteredMap{ 0 };      ///< IBL预滤波环境贴图
	GLuint mBRDFLUT{ 0 };             ///< BRDF积分查找表

 // 是否使用贴图
	bool mUseAlbedoMap{ true };
	bool mUseNormalMap{ true };
	bool mUseMetallicMap{ true };
	bool mUseRoughnessMap{ true };
	bool mUseAOMap{ true };
	bool mUseIBL{ true };

	// 常量参数（对应贴图关闭时生效）
	glm::vec3 mAlbedoValue{ 1.0f, 1.0f, 1.0f };
	float mMetallicValue{ 0.0f };
	float mRoughnessValue{ 0.5f };
	float mAOValue{ 1.0f };
};