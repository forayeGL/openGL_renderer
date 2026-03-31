#version 460 core
out vec4 FragColor;



in vec2 uv;
in vec3 normal;
in vec3 worldPosition;

// UBOs
#include "../common/lightUBO.glsl"
#include "../common/renderSettings.glsl"

uniform sampler2D sampler;
uniform sampler2D specularMaskSampler;

uniform float shiness;

// ===== Phong lighting (UBO-based) =====

vec3 calcDiffuse(vec3 lightColor, vec3 objColor, vec3 lightDir, vec3 N) {
	float diff = clamp(dot(-lightDir, N), 0.0, 1.0);
	return lightColor * diff * objColor;
}

vec3 calcSpecular(vec3 lightColor, vec3 lightDir, vec3 N, vec3 V, float specIntensity) {
	float flag = step(0.0, dot(-lightDir, N));
	vec3 R = normalize(reflect(lightDir, N));
	float spec = pow(max(dot(R, -V), 0.0), shiness);
	return lightColor * spec * flag * specIntensity;
}

vec3 calcDirectionalLight(vec3 objColor, vec3 N, vec3 V) {
	vec3 dir = normalize(dirLight.direction.xyz);
	vec3 col = dirLight.color.xyz * dirLight.color.w;
	float specI = dirLight.specularPad.x;
	return calcDiffuse(col, objColor, dir, N) + calcSpecular(col, dir, N, V, specI);
}

vec3 calcPointLight(GPUPointLight pl, vec3 objColor, vec3 N, vec3 V) {
 vec3 toLight = worldPosition - pl.position.xyz;
	float dist = length(toLight);
	float range = pl.params.x;
	if (range > 0.0 && dist > range) {
		return vec3(0.0);
	}

	vec3 lightDir = toLight / max(dist, 0.0001);
	float specI = pl.attenuation.x;
	float k2 = pl.attenuation.y;
	float k1 = pl.attenuation.z;
	float kc = pl.attenuation.w;
    float atten = 1.0 / max(k2 * dist * dist + k1 * dist + kc, 0.0001);
	return (calcDiffuse(pl.color.xyz, objColor, lightDir, N)
		  + calcSpecular(pl.color.xyz, lightDir, N, V, specI)) * atten;
}

void main()
{
	if (renderMode == 2) {
		FragColor = vec4(1.0);
		return;
	}

	vec3 objectColor = texture(sampler, uv).xyz;
	vec3 N = normalize(normal);
	vec3 V = normalize(worldPosition - uCameraPosition.xyz);

	vec3 result = calcDirectionalLight(objectColor, N, V);

	int nPL = numPointLights;
	for (int i = 0; i < nPL && i < MAX_POINT_LIGHTS; i++) {
		result += calcPointLight(pointLights[i], objectColor, N, V);
	}

	float alpha = texture(sampler, uv).a;
	vec3 ambient = objectColor * ambientColor.xyz;
	vec3 finalColor = result + ambient;

	FragColor = vec4(finalColor, alpha * uOpacity);
}