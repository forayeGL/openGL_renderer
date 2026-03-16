#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 4) in mat4 aInstanceMatrix;

uniform mat4 lightMatrix;

out vec3 worldPosition;

void main() {
	worldPosition = (aInstanceMatrix * vec4(aPos, 1.0)).xyz;
	gl_Position = lightMatrix * aInstanceMatrix * vec4(aPos, 1.0);
}