#pragma once
#include "../scene.h"
#include "../../application/camera/camera.h"
#include "../light/directionalLight.h"
#include "../light/pointLight.h"
#include "../core.h"
#include <vector>

/**
 * @brief 渲染上下文
 * 
 * 将每帧渲染所需的所有场景数据打包为一个结构体，
 * 传递给IRenderPipeline::execute()，使管线与场景管理解耦。
 * 
 * 包含：
 * - 场景引用（主场景 + 后处理场景）
 * - 相机
 * - 光源（方向光 + 点光源）
 * - IBL资源句柄
 * - 管线级设置（清屏色、环境光色、渲染模式）
 */
struct RenderContext {
	Scene*                       mainScene{ nullptr };
	Scene*                       postScene{ nullptr };
	Camera*                      camera{ nullptr };
	DirectionalLight*            dirLight{ nullptr };
	std::vector<PointLight*>*    pointLights{ nullptr };

	// 管线级设置（由GUI写入，管线消费）
	glm::vec3 clearColor{};
	glm::vec3 ambientColor{ 0.15f };
	int       renderModeIdx{ 0 };
	int       shadowType{ 1 };        // 0: Normal, 1: PCF, 2: CSM
	bool      enableAxis{ true };     ///< 是否启用坐标系渲染
	bool      enableTAA{ false };     ///< 是否启用TAA
	float     taaBlendFactor{ 0.1f }; ///< 历史混合系数（越大越平滑）
	bool      taaNeighborhoodClamp{ true }; ///< 是否启用邻域裁剪抑制拖影
	bool      taaResetHistory{ false }; ///< 本帧是否强制重置TAA历史

	// IBL资源句柄（由应用层设置，延迟管线消费）
	GLuint iblIrradianceMap{ 0 };     ///< IBL辐照度立方体贴图
	GLuint iblPrefilteredMap{ 0 };    ///< IBL预滤波环境贴图
	GLuint iblBRDFLUT{ 0 };           ///< BRDF积分查找表
};
