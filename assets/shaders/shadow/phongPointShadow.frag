#version 460 core
out vec4 FragColor;

in vec2 uv;
in vec3 normal;
in vec3 worldPosition;

// UBOs
#include "../common/lightUBO.glsl"
#include "../common/shadowUBO.glsl"
#include "../common/renderSettings.glsl"

uniform sampler2D sampler;
uniform samplerCube pointShadowMaps[MAX_POINT_SHADOW];

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

#define NUM_SAMPLES 32
#define PI 3.141592653589793
#define PI2 6.283185307179586

float rand_2to1(vec2 uv ) { 
  // 0 - 1
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

vec3 disk[NUM_SAMPLES];
void poissonDiskSamples(vec2 randomSeed){
	//1 初始弧度
	float angleTheta = rand_2to1(randomSeed) * PI2;
	float angleFi = rand_2to1(randomSeed) * PI2;

	//2 初始半径
	float radius = 1.0 / float(NUM_SAMPLES);

	//3 弧度步长
	float angleStep = 3.883222077450933;

	//4 半径步长
	float radiusStep = radius;

	//5 循环生成
	for(int i = 0;i < NUM_SAMPLES;i++){
		disk[i] = vec3(sin(angleTheta)*cos(angleFi), cos(angleTheta),sin(angleTheta) * sin(angleFi)) * pow(radius, shadowParams1.z);
		radius += radiusStep;
		angleTheta += angleStep;
		angleFi += angleStep;
	}
}

float getBias(vec3 normal, vec3 lightDir){
	vec3 normalN = normalize(normal);
	vec3 lightDirN = normalize(lightDir);
	
	return max(shadowParams1.x * (1.0 - dot(normalN, lightDirN)), 0.0005);
}


float getPointShadowFar(int i) {
	if (i < 4) return pointShadowFar01[i];
	return pointShadowFar23[i - 4];
}

float pcfPoint(int lightIdx, vec3 lightPos, vec3 N) {
	vec3 lightDis = worldPosition - lightPos;
	vec3 uvw = normalize(lightDis);
	float farVal = getPointShadowFar(lightIdx);
	float depth = length(lightDis) / (1.414 * farVal);

	poissonDiskSamples(uvw.xy);

	vec3 lightDir = normalize(lightDis);
	float biasVal = getBias(N, -lightDir);

	float pcfR = shadowParams1.y;
	float sum = 0.0;
	for (int i = 0; i < NUM_SAMPLES; i++) {
		float closest = texture(pointShadowMaps[lightIdx], uvw + disk[i] * pcfR).r;
		sum += (depth - biasVal) > closest ? 1.0 : 0.0;
	}
	return sum / float(NUM_SAMPLES);
}

void main()
{
	vec3 objectColor = texture(sampler, uv).xyz;
	vec3 N = normalize(normal);
	vec3 V = normalize(worldPosition - uCameraPosition.xyz);

	vec3 result = vec3(0.0);
	int nPL = numPointLights;
	int nPS = shadowFlags.y;
	for (int i = 0; i < nPL && i < MAX_POINT_LIGHTS; i++) {
		vec3 plResult = calcPointLight(pointLights[i], objectColor, N, V);
		float plShadow = 0.0;
		if (i < nPS) {
			plShadow = pcfPoint(i, pointLights[i].position.xyz, N);
		}
		result += plResult * (1.0 - plShadow);
	}

	float alpha = texture(sampler, uv).a;
	vec3 ambient = objectColor * ambientColor.xyz;
	vec3 finalColor = result + ambient;

	FragColor = vec4(finalColor, alpha * uOpacity);
}