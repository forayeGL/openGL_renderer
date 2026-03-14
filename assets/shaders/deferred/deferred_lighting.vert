#version 460 core

// ============================================================================
// 延迟渲染光照阶段顶点着色器
// 
// 简单的全屏四边形Pass：直接将2D坐标映射到NDC空间
// ============================================================================

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aUV;

out vec2 vUV;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0);
    vUV = aUV;
}
