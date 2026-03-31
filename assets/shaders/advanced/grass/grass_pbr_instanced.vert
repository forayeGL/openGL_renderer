#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aColor;
layout (location = 4) in mat4 aInstanceMatrix;

out vec2 vUV;
out vec3 vWorldPosition;
out vec3 vNormal;
out vec2 vWorldXZ;
out float vBendWeight;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

uniform float time;
uniform vec3  windDirection;
uniform float windAmplitude;
uniform float windFrequency;
uniform float windGustAmplitude;
uniform float windGustFrequency;
uniform float phaseScale;

void main() {
	mat4 worldMatrix = modelMatrix * aInstanceMatrix;
	vec4 worldPos = worldMatrix * vec4(aPos, 1.0);

	vec3 windDir = normalize(vec3(windDirection.x, 0.0, windDirection.z));
	float phase = dot(worldPos.xz, normalize(max(abs(windDir.xz), vec2(0.001)))) / max(phaseScale, 0.001);
	float baseOsc = sin(time * windFrequency + phase);
	float gustOsc = sin(time * windGustFrequency + phase * 1.7);

	float heightMask = clamp(aPos.y, 0.0, 1.0);
	float bendWeight = clamp(1.0 - aColor.r, 0.0, 1.0) * heightMask;
	float bend = baseOsc * windAmplitude + gustOsc * windGustAmplitude;
	worldPos.xyz += windDir * (bend * bendWeight);

	mat3 nMat = transpose(inverse(mat3(worldMatrix)));
	vec3 N = normalize(nMat * aNormal);
	N = normalize(N + windDir * (bend * bendWeight * 0.35));

	vUV = aUV;
	vWorldPosition = worldPos.xyz;
	vNormal = N;
	vWorldXZ = worldPos.xz;
	vBendWeight = bendWeight;

	gl_Position = projectionMatrix * viewMatrix * worldPos;
}
