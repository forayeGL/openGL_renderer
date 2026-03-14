#version 460 core

// ============================================================================
// GBuffer几何填充顶点着色器
// 
// 功能：将顶点从模型空间变换到裁剪空间，同时输出世界空间的位置、
//       法线和TBN矩阵供片段着色器使用。
// ============================================================================

layout (location = 0) in vec3 aPos;       // 顶点位置
layout (location = 1) in vec2 aUV;        // 纹理坐标
layout (location = 2) in vec3 aNormal;    // 顶点法线
layout (location = 3) in vec3 aTangent;   // 顶点切线

// 输出到片段着色器
out vec2 vUV;
out vec3 vWorldPosition;
out vec3 vNormal;
out mat3 vTBN;

// 变换矩阵
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat3 normalMatrix;

void main()
{
    // 计算世界空间位置
    vec4 worldPos = modelMatrix * vec4(aPos, 1.0);
    vWorldPosition = worldPos.xyz;

    // 输出裁剪空间位置
    gl_Position = projectionMatrix * viewMatrix * worldPos;

    // 传递纹理坐标
    vUV = aUV;

    // 计算世界空间法线（使用法线矩阵保证非均匀缩放正确性）
    vNormal = normalize(normalMatrix * aNormal);

    // 构建TBN矩阵（切线空间到世界空间的变换）
    vec3 T = normalize(mat3(modelMatrix) * aTangent);
    vec3 N = vNormal;
    vec3 B = normalize(cross(N, T));
    vTBN = mat3(T, B, N);
}
