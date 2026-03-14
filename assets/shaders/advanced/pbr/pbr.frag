#version 460 core
in vec2 uv;
in vec3 normal;
in vec3 worldPosition;
in mat3 tbn;

out vec4 FragColor;

// pbr.frag is located at assets/shaders/advanced/pbr/,
// so common/ is two levels up from here.
#include "../../common/lightUBO.glsl"
#include "../../common/shadowUBO.glsl"
#include "../../common/renderSettings.glsl"
#include "../../common/shadowPCF.glsl"

uniform sampler2D albedoTex;
uniform sampler2D roughnessTex;
uniform sampler2D metallicTex;
uniform sampler2D normalTex;
uniform sampler2D aoTex;

// ==========================================
// 阴影贴图
// ==========================================
uniform sampler2D   shadowMapSampler;                    // 方向光阴影贴图
uniform samplerCube pointShadowMaps[MAX_POINT_SHADOW];   // 点光源立方体阴影贴图

uniform int         useNormalMap;
uniform int         useIBL;
uniform samplerCube irradianceMap;   // diffuse IBL
uniform samplerCube prefilteredMap;  // specular IBL (pre-filtered environment)
uniform sampler2D   brdfLUT;         // specular IBL BRDF integration map

#define PI                3.141592653589793
#define MAX_REFLECTION_LOD 4.0

float getShadowPcfRadius() { return shadowParams1.y; }
int   getHasDirShadow()    { return shadowFlags.x; }
int   getNumPointShadows() { return shadowFlags.y; }

float getPointShadowFar(int i) {
    if (i < 4) return pointShadowFar01[i];
    return pointShadowFar23[i - 4];
}

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

// ============================================================
// Cook-Torrance BRDF helpers
// ============================================================

// GGX Normal Distribution Function
float NDF_GGX(vec3 N, vec3 H, float roughness) {
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;
    return a2 / max(denom, 0.0001);
}

// Schlick-GGX single term (direct lighting remapping)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 0.00001);
}

// Smith's method combining view and light geometry terms
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness)
         * GeometrySchlickGGX(NdotL, roughness);
}

// Fresnel-Schlick for direct lighting
vec3 fresnelSchlick(vec3 F0, float cosTheta) {
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// Fresnel-Schlick with roughness attenuation for IBL ambient
vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0)
              * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

// ============================================================
// Per-light direct radiance contributions
// ============================================================

vec3 calcDirLightPBR(vec3 N, vec3 V,
                     vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3 L = normalize(-dirLight.direction.xyz);
    vec3 H = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float D = NDF_GGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));

    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    vec3 radiance = dirLight.color.xyz * dirLight.color.w;
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 calcPointLightPBR(GPUPointLight pl, vec3 N, vec3 V,
                       vec3 albedo, float metallic, float roughness, vec3 F0) {
    vec3  L     = normalize(pl.position.xyz - worldPosition);
    vec3  H     = normalize(L + V);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);

    float dist        = length(pl.position.xyz - worldPosition);
    float attenuation = 1.0 / (dist * dist);

    float D = NDF_GGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));

    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
    vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

    vec3 radiance = pl.color.xyz * attenuation;
    return (kD * albedo / PI + specular) * radiance * NdotL;
}

// ============================================================
// Main
// ============================================================

void main()
{
    // Unpack normal from tangent space when normal map is available
    vec3 N;
    if (useNormalMap != 0) {
        vec3 normalSample = texture(normalTex, uv).rgb * 2.0 - 1.0;
        N = normalize(tbn * normalSample);
    } else {
        N = normalize(normal);
    }

    if (renderMode == 2) {
        float dirShadow = calcDirectionalShadow(worldPosition, N);
        float totalShadow = dirShadow;
        int nPL = numPointLights;
        for (int i = 0; i < nPL && i < MAX_POINT_LIGHTS; i++) {
            totalShadow = min(totalShadow, calcPointShadow(i, worldPosition, pointLights[i].position.xyz, N));
        }
        FragColor = vec4(vec3(totalShadow), 1.0);
        return;
    }

    // Sample PBR textures
    vec3  albedo    = texture(albedoTex, uv).xyz;
    float metallic  = texture(metallicTex, uv).b;
    float roughness = texture(roughnessTex, uv).r;
    float ao        = texture(aoTex, uv).r;

    vec3 V  = normalize(uCameraPosition.xyz - worldPosition);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // --- Direct lighting ---
    vec3 Lo = vec3(0.0);

    float dirShadow = calcDirectionalShadow(worldPosition, N);
    Lo += calcDirLightPBR(N, V, albedo, metallic, roughness, F0) * dirShadow;

    for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; i++) {
        float ptShadow = calcPointShadow(i, worldPosition, pointLights[i].position.xyz, N);
        Lo += calcPointLightPBR(pointLights[i], N, V, albedo, metallic, roughness, F0) * ptShadow;
    }

    // --- Ambient / IBL ---
    vec3 ambient;
    if (useIBL != 0) {
        float NdotV = max(dot(N, V), 0.0);
        vec3  F     = fresnelSchlickRoughness(F0, NdotV, roughness);
        vec3  kD    = (vec3(1.0) - F) * (1.0 - metallic);

        // Diffuse IBL: irradiance map pre-integrates all incoming light directions
        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuseIBL = irradiance * albedo * kD;

        // Specular IBL: split-sum approximation
        vec3 R = reflect(-V, N);
        vec3 prefilteredColor = textureLod(prefilteredMap, R, roughness * MAX_REFLECTION_LOD).rgb;
        vec2 brdf             = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        vec3 specularIBL      = prefilteredColor * (F * brdf.x + brdf.y);

        ambient = (diffuseIBL + specularIBL) * ao;
    } else {
        ambient = ambientColor.xyz * albedo * ao;
    }

    vec3 color = ambient + Lo;
    FragColor = vec4(color, uOpacity);
}