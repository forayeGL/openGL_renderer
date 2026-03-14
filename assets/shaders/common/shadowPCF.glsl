
#define NUM_SAMPLES 32
#define PI 3.141592653589793
#define PI2 6.283185307179586

float rand_2to1(vec2 uv ) { 
    const highp float a = 12.9898, b = 78.233, c = 43758.5453;
    highp float dt = dot(uv.xy, vec2(a, b)), sn = mod(dt, PI);
    return fract(sin(sn) * c);
}

vec2 disk2D[NUM_SAMPLES];
void poissonDiskSamples2D(vec2 randomSeed){
float angle = rand_2to1(randomSeed) * PI2;
float radius = 1.0 / float(NUM_SAMPLES);
float angleStep = 3.883222077450933;
float radiusStep = radius;
for(int i = 0;i < NUM_SAMPLES;i++){
disk2D[i] = vec2(cos(angle), sin(angle)) * pow(radius, shadowParams1.z);
radius += radiusStep;
angle += angleStep;
}
}

vec3 disk3D[NUM_SAMPLES];
void poissonDiskSamples3D(vec2 randomSeed){
float angleTheta = rand_2to1(randomSeed) * PI2;
float angleFi = rand_2to1(randomSeed) * PI2;
float radius = 1.0 / float(NUM_SAMPLES);
float angleStep = 3.883222077450933;
float radiusStep = radius;
for(int i = 0;i < NUM_SAMPLES;i++){
disk3D[i] = vec3(sin(angleTheta)*cos(angleFi), cos(angleTheta), sin(angleTheta)*sin(angleFi)) * pow(radius, shadowParams1.z);
radius += radiusStep;
angleTheta += angleStep;
angleFi += angleStep;
}
}

float getDirBias(vec3 normal, vec3 lightDir){
vec3 normalN = normalize(normal);
vec3 lightDirN = normalize(lightDir);
return max(shadowParams1.x * (1.0 - dot(normalN, lightDirN)), 0.0002);
}

// CSM Uniforms
uniform int csmLayerCount;
uniform float csmLayers[20];
uniform mat4 lightMatrices[20];
uniform mat4 viewMatrix;
uniform sampler2DArray csmShadowMapSampler;

int getCurrentLayer(vec3 positionWorldSpace){
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

float pcfCSM(vec4 lightSpaceClipCoord, int layer, vec3 normal, vec3 lightDir, float pcfUVRadius){
vec3 lightNDC = lightSpaceClipCoord.xyz/lightSpaceClipCoord.w;
vec3 projCoord = lightNDC * 0.5 + 0.5;
vec2 uv = projCoord.xy;
float depth = projCoord.z;

poissonDiskSamples2D(uv);

float bias = getDirBias(normal, lightDir);
float sum = 0.0;
for(int i = 0;i < NUM_SAMPLES;i++){
float closestDepth = texture(csmShadowMapSampler, vec3(uv + disk2D[i] * pcfUVRadius, layer)).r;
sum += closestDepth < (depth - bias) ? 1.0 : 0.0;
}
return sum / float(NUM_SAMPLES);
}

