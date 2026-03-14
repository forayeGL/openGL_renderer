#version 460 core
layout (location = 0) in vec3 aPos;
out vec3 uvw;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main(){
    vec4 transformPosition = vec4(aPos, 1.0);
    gl_Position = projectionMatrix * viewMatrix * transformPosition;
    
    uvw = normalize(aPos);
}