#version 460 core

out vec4 FragColor;
in vec2 vUV;

uniform sampler2D currentTex;
uniform sampler2D historyTex;
uniform float historyBlend;
uniform int neighborhoodClamp;
uniform float texelSizeX;
uniform float texelSizeY;

vec3 clampHistory(vec2 uv, vec3 historyColor) {
	vec3 cMin = vec3(1e9);
	vec3 cMax = vec3(-1e9);

	for (int y = -1; y <= 1; ++y) {
		for (int x = -1; x <= 1; ++x) {
			vec2 offset = vec2(float(x) * texelSizeX, float(y) * texelSizeY);
			vec3 sampleColor = texture(currentTex, uv + offset).rgb;
			cMin = min(cMin, sampleColor);
			cMax = max(cMax, sampleColor);
		}
	}

	return clamp(historyColor, cMin, cMax);
}

void main() {
	vec3 currentColor = texture(currentTex, vUV).rgb;
	vec3 historyColor = texture(historyTex, vUV).rgb;

	if (neighborhoodClamp != 0) {
		historyColor = clampHistory(vUV, historyColor);
	}

	float w = clamp(historyBlend, 0.0, 0.98);
	vec3 taaColor = mix(currentColor, historyColor, w);
	FragColor = vec4(taaColor, 1.0);
}
