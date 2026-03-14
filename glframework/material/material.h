#pragma once
#include "../core.h"
#include <vector>

class Shader;
class Mesh;
class Camera;
class PointLight;

//使用C++的枚举类型
/**
 * @brief 材质类型枚举
 * 
 * 标识各种材质类型，用于运行时类型判断和Shader选择。
 */
enum class MaterialType {
	PhongMaterial,
	WhiteMaterial,
	DepthMaterial,
	OpacityMaskMaterial,
	ScreenMaterial,
	CubeMaterial,
	PhongEnvMaterial,
	PhongInstanceMaterial,
	GrassInstanceMaterial,

	PhongNormalMaterial,
	PhongParallaxMaterial,
	PhongShadowMaterial,
	PhongCSMShadowMaterial,
	PhongPointShadowMaterial,
	PbrMaterial,
	IBLMaterial,

	// 延迟渲染专用材质类型
	DeferredPbrMaterial
};

class Material {
public:
	Material();
	virtual ~Material();

	virtual const char* getVertexShaderPath() const = 0;
	virtual const char* getFragmentShaderPath() const = 0;

	virtual void applyUniforms(
		Shader* shader,
		Mesh* mesh,
		Camera* camera,
		const std::vector<PointLight*>& pointLights
	) = 0;

public:
	MaterialType mType;

	//深度检测相关
	bool	mDepthTest{ true };
	GLenum	mDepthFunc{ GL_LEQUAL };
	bool	mDepthWrite{ true };

	//polygonOffset相关
	bool			mPolygonOffset{ false };
	unsigned int	mPolygonOffsetType{ GL_POLYGON_OFFSET_FILL };
	float			mFactor{ 0.0f };
	float			mUnit{ 0.0f };

	//stencil相关
	//glEnable(GL_STENCIL_TEST);
	//glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	//glStencilMask(0xFF);//保证了模板缓冲可以被清理
	//glStencilFunc(GL_ALWAYS, 1, 0xFF);//如何测试
	bool			mStencilTest{ false };

	unsigned int	mSFail{ GL_KEEP };	//模板测试失败了怎么办
	unsigned int	mZFail{ GL_KEEP };	//模板测试通过但是深度检测没通过怎么办
	unsigned int	mZPass{ GL_KEEP };	//模板+深度测试都通过了怎么办

	unsigned int	mStencilMask{ 0xFF };//用于控制模板写入

	unsigned int    mStencilFunc{ GL_ALWAYS };
	unsigned int	mStencilRef{ 0 };
	unsigned int	mStencilFuncMask{ 0xFF };

	//颜色混合
	bool			mBlend{ false };
	unsigned int	mSFactor{ GL_SRC_ALPHA };
	unsigned int	mDFactor{ GL_ONE_MINUS_SRC_ALPHA };

	float			mOpacity{ 1.0f };

	//Face Culling
	bool			mFaceCulling{ false };
	unsigned int	mFrontFace{ GL_CCW };
	unsigned int	mCullFace{ GL_BACK };
};