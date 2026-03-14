#version 460 core
out vec4 FragColor;

in vec2 uv;
in vec3 normal;
in vec3 worldPosition;
in mat3 tbn;

// UBOs
#include "../../common/lightUBO.glsl"
#include "../../common/renderSettings.glsl"


uniform sampler2D albedoTex;
uniform sampler2D roughnessTex;
uniform sampler2D metallicTex;
uniform sampler2D normalTex;

uniform samplerCube irradianceMap;
uniform int useIBL;

#define PI 3.141592653589793





//NDF：α本应该表示粗糙度（roughness），α= r^2
float NDF_GGX(vec3 N, vec3 H, float roughness){
	float a = roughness * roughness;
	float a2 = a*a;
	float NdotH = max(dot(N,H), 0.0);

	float num = a2;
	float denom = (NdotH * NdotH * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

	denom = max(denom, 0.0001);

	return num / denom;
}

//Geometry
float GeometrySchlickGGX(float NdotV, float roughness){
	float r = (roughness + 1.0);
	float k = r * r / 8.0;

	float num = NdotV;
	float denom = NdotV * (1.0 - k) + k;

	denom = max(denom, 0.00001);

	return num / denom;
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness){
	float NdotV = max(dot(N,V),0.0);
	float NdotL = max(dot(N,L),0.0);

	float ggx1 = GeometrySchlickGGX(NdotV, roughness);
	float ggx2 = GeometrySchlickGGX(NdotL, roughness);

	return ggx1 * ggx2;
}

//Fresnel
vec3 fresnelSchlick(vec3 F0, float HdotV){
	return F0 + (1.0 - F0) * pow((1.0 - HdotV), 5.0);
}

vec3 fresnelSchlickRoughness(vec3 F0,float HdotV, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow((1.0 - HdotV), 5.0);
}   

void main()
{
	if (renderMode == 2) {
		FragColor = vec4(1.0);
		return;
	}

	//1 准备通用数据
	vec3 albedo  = texture(albedoTex, uv).xyz ;
	vec3 V = normalize(uCameraPosition.xyz - worldPosition);
	vec3 N = texture(normalTex, uv).rgb;
	N = N * 2.0 - 1.0;
	N = normalize(tbn * N);
	float metallic = texture(metallicTex, uv).b;
	float roughness = texture(roughnessTex, uv).r;

	//2 计算基础反射率
	vec3 F0 = vec3(0.04);
	F0 = mix(F0, albedo, metallic);

	//3 遍历点光源，计算反射总共的radiance
	vec3 Lo = vec3(0.0);
	int nPL = numPointLights;
	for(int i = 0; i < nPL && i < MAX_POINT_LIGHTS; i++){
		vec3 L = normalize(pointLights[i].position.xyz - worldPosition);
		vec3 H = normalize(L + V);
		float NdotL = max(dot(N,L),0.0);
		float NdotV = max(dot(N,V),0.0);

		float dis = length(pointLights[i].position.xyz - worldPosition);
		float attenuation = 1.0 / (dis * dis);
		vec3 irradiance = pointLights[i].color.xyz * NdotL * attenuation; 

		float D = NDF_GGX(N,H,roughness);
		float G = GeometrySmith(N,V,L,roughness);
		vec3 F = fresnelSchlick(F0,max(dot(H,V),0.0));

		vec3 ks = F;
		vec3 kd = vec3(1.0) - ks;
		kd *= (1.0 - metallic);

		vec3 num = D * G * F;
		float denom = max(4.0 * NdotL * NdotV, 0.0000001);
		vec3 specularBRDF = num / denom; 

		Lo += (kd * albedo / PI + specularBRDF) * irradiance;
	}

	// Directional light contribution
	{
		vec3 L = normalize(-dirLight.direction.xyz);
		vec3 H = normalize(L + V);
		float NdotL = max(dot(N, L), 0.0);
		float NdotV = max(dot(N, V), 0.0);

		float D = NDF_GGX(N, H, roughness);
		float G = GeometrySmith(N, V, L, roughness);
		vec3  F = fresnelSchlick(F0, max(dot(H, V), 0.0));

		vec3 ks = F;
		vec3 kd = vec3(1.0) - ks;
		kd *= (1.0 - metallic);

		vec3 num   = D * G * F;
		float denom = max(4.0 * NdotL * NdotV, 0.0000001);
		vec3 specularBRDF = num / denom;

		vec3 lightRadiance = dirLight.color.xyz * dirLight.color.w * NdotL;
		Lo += (kd * albedo / PI + specularBRDF) * lightRadiance;
	}

	vec3 ambient;
	if (useIBL > 0) {
		vec3 envIrradiance = texture(irradianceMap, N).rgb;
		vec3 kd = 1.0 - fresnelSchlickRoughness(F0,max(dot(N,V),0.0), roughness);
		ambient = envIrradiance * albedo * kd;
	} else {
		ambient = ambientColor.rgb * albedo;
	}

	Lo += ambient;

	FragColor = vec4(Lo, 1.0);
}