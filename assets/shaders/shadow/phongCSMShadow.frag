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
uniform sampler2D specularMaskSampler;
uniform sampler2DArray shadowMapSampler;

uniform float shiness;

#define NUM_SAMPLES 32
#define PI 3.141592653589793
#define PI2 6.283185307179586

float rand_2to1(vec2 uv ) { 
  // 0 - 1
	const highp float a = 12.9898, b = 78.233, c = 43758.5453;
	highp float dt = dot( uv.xy, vec2( a,b ) ), sn = mod( dt, PI );
	return fract(sin(sn) * c);
}

vec2 disk[NUM_SAMPLES];
void poissonDiskSamples(vec2 randomSeed){
	//1 初始弧度
	float angle = rand_2to1(randomSeed) * PI2;

	//2 初始半径
	float radius = 1.0 / float(NUM_SAMPLES);

	//3 弧度步长
	float angleStep = 3.883222077450933;

	//4 半径步长
	float radiusStep = radius;

	//5 循环生成
	for(int i = 0;i < NUM_SAMPLES;i++){
		disk[i] = vec2(cos(angle), sin(angle)) * pow(radius, shadowParams1.z);
		radius += radiusStep;
		angle += angleStep;
	}
}



float getBias(vec3 normal, vec3 lightDir){
	vec3 normalN = normalize(normal);
	vec3 lightDirN = normalize(lightDir);

	return max(shadowParams1.x * (1.0 - dot(normalN, lightDirN)), 0.0002);
}

float pcf(vec4 lightSpaceClipCoord, int layer, vec3 normal, vec3 lightDir, float pcfUVRadius){
	//1 找到当前像素在光源空间内的NDC坐标
	vec3 lightNDC = lightSpaceClipCoord.xyz/lightSpaceClipCoord.w;

	//2 找到当前像素在ShadowMap上的uv
	vec3 projCoord = lightNDC * 0.5 + 0.5;
	vec2 uv = projCoord.xy;
	float depth = projCoord.z;

	poissonDiskSamples(uv);

	//3  遍历poisson采样盘的每一个采样点，每一个的深度值都需要与当前像素在光源下的深度值进行对比
	float sum = 0.0;
	for(int i = 0;i < NUM_SAMPLES;i++){
		float closestDepth = texture(shadowMapSampler,vec3(uv + disk[i] * pcfUVRadius,layer)).r;
		sum += closestDepth < (depth - getBias(normal, lightDir))?1.0:0.0;
	}

	return sum / float(NUM_SAMPLES);
}

uniform int csmLayerCount;
uniform float csmLayers[20];
uniform mat4 lightMatrices[20];
uniform mat4 viewMatrix;

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

int getCurrentLayer(vec3 positionWorldSpace){
	//求当前像素在相机坐标系下的坐标
	vec3 positionCameraSpace = (viewMatrix * vec4(positionWorldSpace, 1.0)).xyz;
	float z = -positionCameraSpace.z;

	int layer = 0;
	for(int i = 0;i <= csmLayerCount;i++){
		if(z < csmLayers[i]){
			layer = i - 1;
			break;
		}
	}

	return layer;
}

float csm(vec3 positionWorldSpace, vec3 normal, vec3 lightDir, float pcfRadius) {
	int layer = getCurrentLayer(positionWorldSpace);
	vec4 lightSpaceClipCoord = lightMatrices[layer] * vec4(positionWorldSpace, 1.0);

	return pcf(lightSpaceClipCoord,layer, normal, lightDir, pcfRadius);
}

void main()
{
	vec3 objectColor = texture(sampler, uv).xyz;
	vec3 N = normalize(normal);
	vec3 V = normalize(worldPosition - uCameraPosition.xyz);

	vec3 result = calcDirectionalLight(objectColor, N, V);

	float alpha = texture(sampler, uv).a;
	vec3 ambient = objectColor * ambientColor.xyz;

	float shadow = csm(worldPosition, normal, -dirLight.direction.xyz, shadowParams1.y);

	vec3 finalColor = result * (1.0 - shadow) + ambient;
	FragColor = vec4(finalColor, alpha * uOpacity);
}