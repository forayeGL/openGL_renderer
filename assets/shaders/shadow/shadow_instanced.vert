#version 460 core
layout(location = 0) in vec3 aPos;
layout(location = 4) in mat4 aInstanceMatrix;

uniform mat4 lightMatrix;

void main() {
	gl_Position = lightMatrix * aInstanceMatrix * vec4(aPos, 1.0);
}