#version 460 core 
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aUV;
layout (location = 2) in vec3 aNormal;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in mat4 aInstanceMatrix;

out vec2 uv;
out vec3 normal;
out vec3 worldPosition;
out mat3 tbn;

uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    vec4 transformPosition = aInstanceMatrix * vec4(aPos, 1.0);
    worldPosition = transformPosition.xyz;
    gl_Position = projectionMatrix * viewMatrix * transformPosition;
    
    uv = aUV;
    
    mat3 normalMatrix = transpose(inverse(mat3(aInstanceMatrix)));
    normal = normalize(normalMatrix * aNormal);
    
    vec3 tangent = normalize(mat3(aInstanceMatrix) * aTangent);
    // Use vNormal or just normal from above (before normalize is fine, but re-normalized is better)
    vec3 N = normal;
    vec3 B = normalize(cross(N, tangent));
    tbn = mat3(tangent, B, N);
}
