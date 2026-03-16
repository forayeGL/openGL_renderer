#version 460 core

layout (location = 0) in vec3 aPos;       
layout (location = 1) in vec2 aUV;        
layout (location = 2) in vec3 aNormal;    
layout (location = 3) in vec3 aTangent;   
layout (location = 4) in mat4 aInstanceMatrix;

out vec2 vUV;
out vec3 vWorldPosition;
out vec3 vNormal;
out mat3 vTBN;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    vec4 worldPos = aInstanceMatrix * vec4(aPos, 1.0);
    vWorldPosition = worldPos.xyz;

    gl_Position = projectionMatrix * viewMatrix * worldPos;

    vUV = aUV;

    mat3 normalMatrix = transpose(inverse(mat3(aInstanceMatrix)));
    vNormal = normalize(normalMatrix * aNormal);

    vec3 T = normalize(mat3(aInstanceMatrix) * aTangent);
    vec3 N = vNormal;
    vec3 B = normalize(cross(N, T));
    vTBN = mat3(T, B, N);
}