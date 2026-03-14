#version 460 core
out vec4 FragColor;

in vec2 uv;
in vec3 normal;
in vec3 worldPosition;
in vec4 lightSpaceClipCoord;
in vec3 lightSpacePosition;

// UBOs must come before any usage of their defines
#include "../common/lightUBO.glsl"
#include "../common/shadowUBO.glsl"
#include "../common/renderSettings.glsl"

uniform sampler2D sampler;
uniform sampler2D specularMaskSampler;
uniform sampler2D shadowMapSampler;
uniform samplerCube pointShadowMaps[MAX_POINT_SHADOW];

uniform float shiness;

// ===== Convenience accessors for packed UBO fields =====

float getShadowBias()        { return shadowParams1.x; }
float getShadowPcfRadius()   { return shadowParams1.y; }
float getShadowDiskTight()   { return shadowParams1.z; }
float getShadowLightSize()   { return shadowParams1.w; }
float getShadowNearPlane()   { return shadowParams2.x; }
float getShadowFrustum()     { return shadowParams2.y; }
int   getHasDirShadow()      { return shadowFlags.x; }
int   getNumPointShadows()   { return shadowFlags.y; }

float getPointShadowFar(int i) {
	if (i < 4) return pointShadowFar01[i];
	return pointShadowFar23[i - 4];
}

// ===== Phong lighting =====

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
	vec3 col = dirLight.color.xyz * dirLight.color.w; // color * intensity
	float specI = dirLight.specularPad.x;
	return calcDiffuse(col, objColor, dir, N) + calcSpecular(col, dir, N, V, specI);
}

vec3 calcPointLight(GPUPointLight pl, vec3 objColor, vec3 N, vec3 V) {
	vec3 lightDir = normalize(worldPosition - pl.position.xyz);
	float dist = length(worldPosition - pl.position.xyz);
	float specI = pl.attenuation.x;
	float k2 = pl.attenuation.y;
	float k1 = pl.attenuation.z;
	float kc = pl.attenuation.w;
	float atten = 1.0 / (k2 * dist * dist + k1 * dist + kc);
	return (calcDiffuse(pl.color.xyz, objColor, lightDir, N)
	      + calcSpecular(pl.color.xyz, lightDir, N, V, specI)) * atten;
}

// ===== Shadow sampling =====

#define NUM_SAMPLES 32
#define PI  3.141592653589793
#define PI2 6.283185307179586

float rand_2to1(vec2 seed) {
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot(seed, vec2(a, b));
	highp float sn = mod(dt, PI);
	return fract(sin(sn) * c);
}

vec2 disk2D[NUM_SAMPLES];
void poissonDiskSamples2D(vec2 seed) {
	float angle = rand_2to1(seed) * PI2;
	float radius = 1.0 / float(NUM_SAMPLES);
	float angleStep = 3.883222077450933;
	float radiusStep = radius;
	for (int i = 0; i < NUM_SAMPLES; i++) {
		disk2D[i] = vec2(cos(angle), sin(angle)) * pow(radius, getShadowDiskTight());
		radius += radiusStep;
		angle += angleStep;
	}
}

vec3 disk3D[NUM_SAMPLES];
void poissonDiskSamples3D(vec2 seed) {
	float angleTheta = rand_2to1(seed) * PI2;
	float angleFi = rand_2to1(seed + vec2(0.5, 0.5)) * PI2;
	float radius = 1.0 / float(NUM_SAMPLES);
	float angleStep = 3.883222077450933;
	float radiusStep = radius;
	for (int i = 0; i < NUM_SAMPLES; i++) {
		disk3D[i] = vec3(sin(angleTheta)*cos(angleFi), cos(angleTheta), sin(angleTheta)*sin(angleFi))
		            * pow(radius, getShadowDiskTight());
		radius += radiusStep;
		angleTheta += angleStep;
		angleFi += angleStep;
	}
}

float getDirBias(vec3 N, vec3 lightDir) {
	return max(getShadowBias() * (1.0 - dot(normalize(N), normalize(lightDir))), 0.0005);
}

float pcfDirectional(vec3 N, vec3 lightDir) {
	vec3 lightNDC = lightSpaceClipCoord.xyz / lightSpaceClipCoord.w;
	vec3 projCoord = lightNDC * 0.5 + 0.5;
	vec2 shadowUV = projCoord.xy;
	float depth = projCoord.z;

	if (depth > 1.0) return 0.0;

	poissonDiskSamples2D(shadowUV);

	float biasVal = getDirBias(N, lightDir);
	float pcfR = getShadowPcfRadius();
	float sum = 0.0;
	for (int i = 0; i < NUM_SAMPLES; i++) {
		float closest = texture(shadowMapSampler, shadowUV + disk2D[i] * pcfR).r;
		sum += (depth - biasVal) > closest ? 1.0 : 0.0;
	}
	return sum / float(NUM_SAMPLES);
}

float pcfPoint(int lightIdx, vec3 lightPos, vec3 N) {
	vec3 lightDis = worldPosition - lightPos;
	vec3 uvw = normalize(lightDis);
	float farVal = getPointShadowFar(lightIdx);
	float depth = length(lightDis) / (1.414 * farVal);

	poissonDiskSamples3D(uvw.xy);

	vec3 lightDir = normalize(lightDis);
	float biasVal = getDirBias(N, -lightDir);

	float pcfR = getShadowPcfRadius();
	float sum = 0.0;
	for (int i = 0; i < NUM_SAMPLES; i++) {
		float closest = texture(pointShadowMaps[lightIdx], uvw + disk3D[i] * pcfR).r;
		sum += (depth - biasVal) > closest ? 1.0 : 0.0;
	}
	return sum / float(NUM_SAMPLES);
}

void main()
{
	vec3 objectColor = texture(sampler, uv).xyz;
	float alpha = texture(sampler, uv).a;
	vec3 N = normalize(normal);
	vec3 V = normalize(worldPosition - uCameraPosition.xyz);

	// Shadow-only mode: just show shadow factor
	if (renderMode == 2) {
		float shadow = 0.0;
		if (getHasDirShadow() == 1) {
			shadow = pcfDirectional(N, dirLight.direction.xyz);
		}
		FragColor = vec4(vec3(1.0 - shadow), 1.0);
		return;
	}

	// Directional light contribution
	vec3 result = calcDirectionalLight(objectColor, N, V);
	float dirShadow = 0.0;
	if (getHasDirShadow() == 1) {
		dirShadow = pcfDirectional(N, dirLight.direction.xyz);
	}
	result *= (1.0 - dirShadow);

	// Point lights contribution
	int nPL = numPointLights;
	for (int i = 0; i < nPL && i < MAX_POINT_LIGHTS; i++) {
		vec3 plResult = calcPointLight(pointLights[i], objectColor, N, V);
		float plShadow = 0.0;
		if (i < getNumPointShadows()) {
			plShadow = pcfPoint(i, pointLights[i].position.xyz, N);
		}
		result += plResult * (1.0 - plShadow);
	}

	// Ambient
	vec3 ambient = objectColor * ambientColor.xyz;
	vec3 finalColor = result + ambient;

	FragColor = vec4(finalColor, alpha * uOpacity);
}