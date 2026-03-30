#pragma once
#include "../core.h"
#include "../light/directionalLight.h"
#include "../light/pointLight.h"
#include "../../application/camera/camera.h"
#include <vector>

// Must match std140 layout in GLSL exactly.
// std140: vec4 = 16B aligned, mat4 = 64B, float[] each element = 16B!
// So we only use vec4/mat4/ivec4 members to guarantee alignment.

static constexpr int MAX_POINT_LIGHTS = 8;
static constexpr int MAX_POINT_SHADOW = 8;

// ---- Render Mode enum (shared with GUI) ----
enum class RenderMode : int {
	Fill       = 0,
	Wireframe  = 1,
	ShadowOnly = 2
};

// ---- GPU structs (std140) ----

struct GPUDirectionalLight {
	glm::vec4 direction;     // xyz = direction
	glm::vec4 color;         // xyz = color, w = intensity
	glm::vec4 specularPad;   // x = specularIntensity
};
// 48 bytes

struct GPUPointLight {
	glm::vec4 position;      // xyz = position
	glm::vec4 color;         // xyz = color
    glm::vec4 attenuation;   // x=specular, y=k2, z=k1, w=kc
	glm::vec4 params;        // x=effectiveRange
};
// 64 bytes

struct LightUBOData {
	GPUDirectionalLight dirLight;                       //   0 -  47
    GPUPointLight       pointLights[MAX_POINT_LIGHTS];  //  48 - 559
	glm::ivec4          counts;                         // 560 - 575  x=numPointLights
};
// 576 bytes

struct ShadowUBOData {
	glm::mat4 lightMatrix;        //   0 -  63
	glm::mat4 lightViewMatrix;    //  64 - 127
	glm::vec4 params1;            // 128 - 143  x=bias, y=pcfRadius, z=diskTightness, w=lightSize
	glm::vec4 params2;            // 144 - 159  x=nearPlane, y=frustum
	glm::vec4 pointShadowFar01;   // 160 - 175  far[0..3]
	glm::vec4 pointShadowFar23;   // 176 - 191  far[4..7]
	glm::ivec4 flags;             // 192 - 207  x=hasDirShadow, y=numPointShadows
};
// 208 bytes

struct RenderSettingsUBOData {
	glm::vec4 ambientColor;       //  0 - 15
	glm::vec4 cameraPosition;     // 16 - 31
	glm::vec4 renderParams;       // 32 - 47  x=opacity, y=renderMode(as float), z=shadowType(as float)
};
// 48 bytes

// ---- UBO binding points (must match GLSL) ----
static constexpr GLuint BINDING_LIGHTS          = 0;
static constexpr GLuint BINDING_SHADOW          = 1;
static constexpr GLuint BINDING_RENDER_SETTINGS = 2;

// ---- Manager ----
class UBOManager {
public:
	UBOManager();
	~UBOManager();

	void init();

	void updateLights(
		DirectionalLight* dirLight,
		const std::vector<PointLight*>& pointLights
	);

	void updateShadow(
		DirectionalLight* dirLight,
		const std::vector<PointLight*>& pointLights
	);

	void updateRenderSettings(
		Camera* camera,
		RenderMode mode,
		int shadowType,
		glm::vec3 ambientColor = glm::vec3(0.15f)
	);

	void bindToShader(GLuint program);

private:
	GLuint mLightUBO{ 0 };
	GLuint mShadowUBO{ 0 };
	GLuint mSettingsUBO{ 0 };

	LightUBOData          mLightData{};
	ShadowUBOData         mShadowData{};
	RenderSettingsUBOData mSettingsData{};
};
