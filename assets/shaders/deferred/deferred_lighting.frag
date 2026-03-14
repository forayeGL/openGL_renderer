#version 460 core

// ============================================================================
// 延迟渲染PBR光照片段着色器
// 
// 从GBuffer读取几何数据，计算：
// 1. Cook-Torrance BRDF 直接光照（方向光 + 点光源）
// 2. IBL间接光照（漫反射辐照度 + 镜面反射预滤波）
// 3. 阴影衰减（方向光PCF + 点光源立方体阴影）
// 
// 与前向渲染PBR相比，延迟渲染将几何和光照分离，
// 光照复杂度与场景物体数量解耦，仅与像素数和光源数相关。
// ============================================================================

out vec4 FragColor;
in vec2 vUV;

// ==========================================
// GBuffer纹理输入
// ==========================================
uniform sampler2D gPosition;      // RT0: 世界空间位置 (rgb)
uniform sampler2D gNormal;        // RT1: 世界空间法线 (rgb)
uniform sampler2D gAlbedo;        // RT2: 反照率 (rgb) + 金属度 (a)
uniform sampler2D gParam;         // RT3: 粗糙度 (r) + AO (g) + 自发光 (b)

// ==========================================
// UBO: 光源、阴影、渲染设置
// ==========================================
#include "../common/lightUBO.glsl"
#include "../common/shadowUBO.glsl"
#include "../common/renderSettings.glsl"
#include "../common/shadowPCF.glsl"

// ==========================================
// 阴影贴图
// ==========================================
uniform sampler2D   shadowMapSampler;                    // 方向光阴影贴图
uniform samplerCube pointShadowMaps[MAX_POINT_SHADOW];   // 点光源立方体阴影贴图

// ==========================================
// IBL纹理
// ==========================================
uniform samplerCube irradianceMap;    // 漫反射辐照度贴图
uniform samplerCube prefilteredMap;   // 镜面反射预滤波环境贴图
uniform sampler2D   brdfLUT;         // BRDF积分查找表
uniform int         useIBL;          // 是否启用IBL

// ==========================================
// 常量定义
// ==========================================
#define PI 3.141592653589793

// ==========================================
// 阴影相关辅助函数
// ==========================================

/// 获取UBO中的阴影参数便捷访问器
float getShadowBias()        { return shadowParams1.x; }
float getShadowPcfRadius()   { return shadowParams1.y; }
float getShadowDiskTight()   { return shadowParams1.z; }
int   getHasDirShadow()      { return shadowFlags.x; }
int   getNumPointShadows()   { return shadowFlags.y; }

float getPointShadowFar(int i) {
    if (i < 4) return pointShadowFar01[i];
    return pointShadowFar23[i - 4];
}

/// 计算方向光阴影
float calcDirectionalShadow(vec3 worldPos, vec3 N) {
    if (getHasDirShadow() == 0) return 1.0;
    vec3 lightDir = normalize(-dirLight.direction.xyz);

    if (shadowType == 2) { // CSM
        int layer = getCurrentLayer(worldPos);
        vec4 lightSpaceClipCoord = lightMatrices[layer] * vec4(worldPos, 1.0);
        float pcfRadius = getShadowPcfRadius();
        float shadow = pcfCSM(lightSpaceClipCoord, layer, N, lightDir, pcfRadius);
        return 1.0 - shadow;
    }

    vec4 lightSpacePos = shadowLightMatrix * vec4(worldPos, 1.0);
    vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 1.0;

    float currentDepth = projCoords.z;
    float biasVal = getDirBias(N, lightDir);
    float depth = currentDepth;

    if (shadowType == 1) { // PCF
        poissonDiskSamples2D(projCoords.xy);
        float pcfR = getShadowPcfRadius();
        float sum = 0.0;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            float closest = texture(shadowMapSampler, projCoords.xy + disk2D[i] * pcfR).r;
            sum += (depth - biasVal) > closest ? 1.0 : 0.0;
        }
        return 1.0 - (sum / float(NUM_SAMPLES));
    } else { // Normal
        float closestDepth = texture(shadowMapSampler, projCoords.xy).r;
        float shadow = currentDepth - biasVal > closestDepth ? 1.0 : 0.0;
        return 1.0 - shadow;
    }
}

/// 计算点光源阴影
float calcPointShadow(int lightIdx, vec3 worldPos, vec3 lightPos, vec3 N) {
    if (lightIdx >= getNumPointShadows()) return 1.0;

    vec3 lightDis = worldPos - lightPos;
    vec3 lightDir = normalize(lightDis);
    vec3 uvw = lightDir;
    float farPlane = getPointShadowFar(lightIdx);
    float depth = length(lightDis) / (1.414 * farPlane);
    float biasVal = getDirBias(N, -lightDir);

    if (shadowType == 1) { // PCF
        poissonDiskSamples3D(uvw.xy);
        float pcfR = getShadowPcfRadius();
        float sum = 0.0;
        for (int i = 0; i < NUM_SAMPLES; i++) {
            float closest = texture(pointShadowMaps[lightIdx], uvw + disk3D[i] * pcfR).r;
            sum += (depth - biasVal) > closest ? 1.0 : 0.0;
        }
        return 1.0 - (sum / float(NUM_SAMPLES));
    } else { // Normal
        float closestDepth = texture(pointShadowMaps[lightIdx], uvw).r;
        float shadow = (depth - biasVal) > closestDepth ? 1.0 : 0.0;
        return 1.0 - shadow;
    }
}

// ==========================================
// PBR BRDF函数
// ==========================================

/// GGX法线分布函数（NDF）
float NDF_GGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

/// Schlick-GGX几何遮蔽函数（单方向）
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = r * r / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

/// Smith几何遮蔽函数（双方向组合）
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

/// Schlick菲涅尔近似
vec3 fresnelSchlick(vec3 F0, float cosTheta) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

/// 带粗糙度的Schlick菲涅尔（用于IBL间接光照）
vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ==========================================
// 光照计算
// ==========================================

/// 计算单个点光源的PBR直接光照贡献
vec3 calcPointLightPBR(GPUPointLight pl, vec3 worldPos, vec3 N, vec3 V,
                        vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 L = normalize(pl.position.xyz - worldPos);
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    // 计算距离衰减
    float dist = length(pl.position.xyz - worldPos);
    float attenuation = 1.0 / (dist * dist);
    vec3 radiance = pl.color.xyz * attenuation * NdotL;

    // Cook-Torrance BRDF
    float D = NDF_GGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));

    // 镜面反射项
    vec3 numerator = D * G * F;
    float denominator = max(4.0 * NdotV * NdotL, 0.0001);
    vec3 specular = numerator / denominator;

    // 漫反射项（金属表面不产生漫反射）
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    return (kD * albedo / PI + specular) * radiance;
}

/// 计算方向光的PBR直接光照贡献
vec3 calcDirLightPBR(vec3 worldPos, vec3 N, vec3 V,
                      vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 lightDir = normalize(dirLight.direction.xyz);
    vec3 L = -lightDir;  // 光线方向取反得到入射方向
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    vec3 radiance = dirLight.color.xyz * dirLight.color.w * NdotL;

    // Cook-Torrance BRDF
    float D = NDF_GGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));

    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    return (kD * albedo / PI + specular) * radiance;
}

// ==========================================
// 主函数
// ==========================================

void main()
{
    // 从GBuffer读取数据
    vec3  worldPos  = texture(gPosition, vUV).rgb;
    vec3  N         = normalize(texture(gNormal, vUV).rgb);
    vec3  albedo    = texture(gAlbedo, vUV).rgb;
    float metallic  = texture(gAlbedo, vUV).a;
    float roughness = texture(gParam, vUV).r;
    float ao        = texture(gParam, vUV).g;

    // 跳过GBuffer中未写入的像素（位置为零向量）
    if (length(worldPos) < 0.001) {
        FragColor = vec4(0.0);
        return;
    }

    if (renderMode == 2) {
        float dirShadow = calcDirectionalShadow(worldPos, N);
        float totalShadow = dirShadow;
        int nPL = numPointLights;
        for (int i = 0; i < nPL && i < MAX_POINT_LIGHTS; i++) {
            totalShadow = min(totalShadow, calcPointShadow(i, worldPos, pointLights[i].position.xyz, N));
        }
        FragColor = vec4(vec3(totalShadow), 1.0);
        return;
    }

    // 视线方向
    vec3 V = normalize(uCameraPosition.xyz - worldPos);

    // 基础反射率F0（介电质默认0.04，金属取albedo）
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // ==========================================
    // 1. 直接光照累加
    // ==========================================
    vec3 Lo = vec3(0.0);

    // 方向光直接光照
    float dirShadow = calcDirectionalShadow(worldPos, N);
    Lo += calcDirLightPBR(worldPos, N, V, albedo, metallic, roughness, F0) * dirShadow;

    // 点光源直接光照
    int nPL = numPointLights;
    for (int i = 0; i < nPL && i < MAX_POINT_LIGHTS; i++) {
        float ptShadow = calcPointShadow(i, worldPos, pointLights[i].position.xyz, N);
        Lo += calcPointLightPBR(pointLights[i], worldPos, N, V, albedo, metallic, roughness, F0)
              * ptShadow;
    }

    // ==========================================
    // 2. IBL间接光照
    // ==========================================
    vec3 ambient = vec3(0.0);
    float NdotV = max(dot(N, V), 0.0);

    if (useIBL > 0) {
        // 漫反射间接光照：从辐照度贴图采样
        vec3 F_ibl = fresnelSchlickRoughness(F0, NdotV, roughness);
        vec3 kD_ibl = (vec3(1.0) - F_ibl) * (1.0 - metallic);
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuseIBL = kD_ibl * irradiance * albedo;

        // 镜面反射间接光照：从预滤波环境贴图 + BRDF LUT采样
        vec3 R = reflect(-V, N);
        const float MAX_REFLECTION_LOD = 4.0;
        vec3 prefilteredColor = textureLod(prefilteredMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 envBRDF = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        vec3 specularIBL = prefilteredColor * (F_ibl * envBRDF.x + envBRDF.y);

        ambient = (diffuseIBL + specularIBL) * ao;
    } else {
        // 无IBL时使用简单环境光
        ambient = ambientColor.rgb * albedo * ao;
    }

    // ==========================================
    // 3. 最终颜色合成
    // ==========================================
    vec3 color = Lo + ambient;

    FragColor = vec4(color, 1.0);
}
