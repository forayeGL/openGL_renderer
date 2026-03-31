#version 460 core

in vec2 vUV;
in vec3 vWorldPosition;
in vec3 vNormal;
in vec2 vWorldXZ;
in float vBendWeight;

out vec4 FragColor;

#include "../../common/lightUBO.glsl"
#include "../../common/shadowUBO.glsl"
#include "../../common/renderSettings.glsl"
#include "../../common/shadowPCF.glsl"

uniform sampler2D albedoTex;
uniform sampler2D normalTex;
uniform sampler2D roughnessTex;
uniform sampler2D metallicTex;
uniform sampler2D aoTex;
uniform sampler2D opacityTex;
uniform sampler2D thicknessTex;

uniform int useAlbedoMap;
uniform int useNormalMap;
uniform int useRoughnessMap;
uniform int useMetallicMap;
uniform int useAOMap;
uniform int useOpacityMap;
uniform int useThicknessMap;
uniform int useTransmission;
uniform int useIBL;

uniform vec3 albedoValue;
uniform float roughnessValue;
uniform float metallicValue;
uniform float aoValue;
uniform float opacityValue;
uniform float thicknessValue;
uniform float specularBoost;
uniform vec3 subsurfaceColor;
uniform float transmissionStrength;
uniform float alphaCutoff;
uniform float uvScale;
uniform float brightness;

uniform vec3 cameraPosition;

uniform sampler2D shadowMapSampler;
uniform samplerCube pointShadowMaps[MAX_POINT_SHADOW];
uniform samplerCube irradianceMap;
uniform samplerCube prefilteredMap;
uniform sampler2D brdfLUT;

#define PI 3.141592653589793
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

	if (shadowType == 2) {
		int layer = getCurrentLayer(worldPos);
		vec4 lightSpaceClipCoord = getCsmLightMatrix(layer) * vec4(worldPos, 1.0);
		float shadow = pcfCSM(lightSpaceClipCoord, layer, N, lightDir, getShadowPcfRadius());
		return 1.0 - shadow;
	}

	vec4 lightSpacePos = shadowLightMatrix * vec4(worldPos, 1.0);
	vec3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
	projCoords = projCoords * 0.5 + 0.5;
	if (projCoords.z > 1.0) return 1.0;

	float depth = projCoords.z;
	float biasVal = getDirBias(N, lightDir);

	if (shadowType == 1) {
		poissonDiskSamples2D(projCoords.xy);
		float sum = 0.0;
		for (int i = 0; i < NUM_SAMPLES; i++) {
			float closest = texture(shadowMapSampler, projCoords.xy + disk2D[i] * getShadowPcfRadius()).r;
			sum += (depth - biasVal) > closest ? 1.0 : 0.0;
		}
		return 1.0 - (sum / float(NUM_SAMPLES));
	}

	float closestDepth = texture(shadowMapSampler, projCoords.xy).r;
	float shadow = depth - biasVal > closestDepth ? 1.0 : 0.0;
	return 1.0 - shadow;
}

float calcPointShadow(int lightIdx, vec3 worldPos, vec3 lightPos, vec3 N) {
	if (lightIdx >= getNumPointShadows()) return 1.0;

	vec3 lightDis = worldPos - lightPos;
	vec3 lightDir = normalize(lightDis);
	vec3 uvw = lightDir;
	float farPlane = getPointShadowFar(lightIdx);
	float depth = length(lightDis) / (1.414 * farPlane);
	float biasVal = getDirBias(N, -lightDir);

	if (shadowType == 1) {
		poissonDiskSamples3D(uvw.xy);
		float sum = 0.0;
		for (int i = 0; i < NUM_SAMPLES; i++) {
			float closest = texture(pointShadowMaps[lightIdx], uvw + disk3D[i] * getShadowPcfRadius()).r;
			sum += (depth - biasVal) > closest ? 1.0 : 0.0;
		}
		return 1.0 - (sum / float(NUM_SAMPLES));
	}

	float closestDepth = texture(pointShadowMaps[lightIdx], uvw).r;
	float shadow = (depth - biasVal) > closestDepth ? 1.0 : 0.0;
	return 1.0 - shadow;
}

float NDF_GGX(vec3 N, vec3 H, float roughness) {
	float a = roughness * roughness;
	float a2 = a * a;
	float NdotH = max(dot(N, H), 0.0);
	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	return a2 / max(PI * denom * denom, 0.0001);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return NdotV / max(NdotV * (1.0 - k) + k, 0.00001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
	float NdotV = max(dot(N, V), 0.0);
	float NdotL = max(dot(N, L), 0.0);
	return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

vec3 fresnelSchlick(vec3 F0, float cosTheta) {
	return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec3 fresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness) {
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}

vec3 calcDirLightPBR(vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0) {
	vec3 L = normalize(-dirLight.direction.xyz);
	vec3 H = normalize(L + V);
	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);

	float D = NDF_GGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = fresnelSchlick(F0, max(dot(H, V), 0.0));
	vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
	vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

	vec3 radiance = dirLight.color.xyz * dirLight.color.w;
	return (kD * albedo / PI + specular) * radiance * NdotL;
}

vec3 calcPointLightPBR(GPUPointLight pl, vec3 N, vec3 V, vec3 albedo, float metallic, float roughness, vec3 F0) {
	vec3 toLight = pl.position.xyz - vWorldPosition;
	float dist = length(toLight);
	float range = pl.params.x;
	if (range > 0.0 && dist > range) {
		return vec3(0.0);
	}

	vec3 L = toLight / max(dist, 0.0001);
	vec3 H = normalize(L + V);
	float NdotL = max(dot(N, L), 0.0);
	float NdotV = max(dot(N, V), 0.0);

	float k2 = pl.attenuation.y;
	float k1 = pl.attenuation.z;
	float kc = pl.attenuation.w;
	float attenuation = 1.0 / max(k2 * dist * dist + k1 * dist + kc, 0.0001);

	float D = NDF_GGX(N, H, roughness);
	float G = GeometrySmith(N, V, L, roughness);
	vec3 F = fresnelSchlick(F0, max(dot(H, V), 0.0));

	vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.0001);
	vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
	vec3 radiance = pl.color.xyz * attenuation;

	return (kD * albedo / PI + specular) * radiance * NdotL;
}

void main() {
	vec2 worldUV = vWorldXZ / max(uvScale, 0.001);

	vec3 albedo = (useAlbedoMap != 0 ? texture(albedoTex, worldUV).rgb : albedoValue) * brightness;
	float roughness = useRoughnessMap != 0 ? texture(roughnessTex, worldUV).r : roughnessValue;
	float metallic = useMetallicMap != 0 ? texture(metallicTex, worldUV).b : metallicValue;
	float ao = useAOMap != 0 ? texture(aoTex, worldUV).r : aoValue;
	float alpha = useOpacityMap != 0 ? texture(opacityTex, vUV).r : opacityValue;
	float thickness = useThicknessMap != 0 ? texture(thicknessTex, vUV).r : thicknessValue;

	if (alpha < alphaCutoff) {
		discard;
	}

	vec3 N = normalize(vNormal);
	if (useNormalMap != 0) {
		vec3 tangentHint = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
		vec3 T = normalize(cross(tangentHint, N));
		vec3 B = normalize(cross(N, T));
		mat3 tbn = mat3(T, B, N);
		vec3 normalSample = texture(normalTex, worldUV).rgb * 2.0 - 1.0;
		N = normalize(tbn * normalSample);
	}

	vec3 V = normalize(uCameraPosition.xyz - vWorldPosition);
	vec3 F0 = mix(vec3(0.04), albedo, metallic);

	float dirShadow = calcDirectionalShadow(vWorldPosition, N);
	vec3 Lo = calcDirLightPBR(N, V, albedo, metallic, roughness, F0) * dirShadow;

	for (int i = 0; i < numPointLights && i < MAX_POINT_LIGHTS; i++) {
		vec3 toLight = pointLights[i].position.xyz - vWorldPosition;
		float dist = length(toLight);
		float range = pointLights[i].params.x;
		if (range > 0.0 && dist > range) continue;

		float ptShadow = calcPointShadow(i, vWorldPosition, pointLights[i].position.xyz, N);
		Lo += calcPointLightPBR(pointLights[i], N, V, albedo, metallic, roughness, F0) * ptShadow;
	}

	vec3 ambient;
	if (useIBL != 0) {
		float NdotV = max(dot(N, V), 0.0);
		vec3 F = fresnelSchlickRoughness(F0, NdotV, roughness);
		vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

		vec3 irradiance = texture(irradianceMap, N).rgb;
		vec3 diffuseIBL = irradiance * albedo * kD;

		vec3 R = reflect(-V, N);
		vec3 prefilteredColor = textureLod(prefilteredMap, R, roughness * MAX_REFLECTION_LOD).rgb;
		vec2 brdf = texture(brdfLUT, vec2(NdotV, roughness)).rg;
		vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

		ambient = (diffuseIBL + specularIBL) * ao;
	} else {
		ambient = ambientColor.xyz * albedo * ao;
	}

	vec3 transmission = vec3(0.0);
	if (useTransmission != 0) {
		vec3 L = normalize(-dirLight.direction.xyz);
		float backLit = pow(max(dot(-N, L), 0.0), 1.5);
		transmission = subsurfaceColor * backLit * thickness * transmissionStrength * (1.0 + vBendWeight * 0.25);
	}

	vec3 color = ambient + Lo + transmission;
	color += vec3(specularBoost) * 0.02;

	FragColor = vec4(color, alpha * uOpacity);
}
