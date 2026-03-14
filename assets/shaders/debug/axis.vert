#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 3) in vec3 aColor;

out vec3 color;

uniform mat4 projectionMatrix;
uniform mat4 viewMatrix;

void main()
{
    color = aColor;
    gl_Position = projectionMatrix * viewMatrix * vec4(aPos, 1.0);
}