#version 460 core

// ============================================================================
// GBuffer几何填充片段着色器
// 
// 将PBR材质属性写入多个渲染目标（MRT）：
//   RT0 (gPosition): 世界空间位置 (rgb) + 备用 (a)
//   RT1 (gNormal):   世界空间法线 (rgb) + 备用 (a)
//   RT2 (gAlbedo):   反照率 (rgb) + 金属度 (a)
//   RT3 (gParam):    粗糙度 (r) + AO (g) + 自发光标志 (b) + 备用 (a)
// ============================================================================

// 多渲染目标输出
layout (location = 0) out vec4 gPosition;   // 世界空间位置
layout (location = 1) out vec4 gNormal;     // 世界空间法线
layout (location = 2) out vec4 gAlbedo;     // 反照率 + 金属度
layout (location = 3) out vec4 gParam;      // 粗糙度 + AO + 自发光

// 来自顶点着色器的输入
in vec2 vUV;
in vec3 vWorldPosition;
in vec3 vNormal;
in mat3 vTBN;

// PBR材质纹理
uniform sampler2D albedoTex;      // 反照率贴图
uniform sampler2D normalTex;      // 法线贴图
uniform sampler2D roughnessTex;   // 粗糙度贴图
uniform sampler2D metallicTex;    // 金属度贴图

// 是否使用法线贴图的标志（当没有法线贴图时使用顶点法线）
uniform int useNormalMap;

void main()
{
    // RT0: 世界空间位置
    gPosition = vec4(vWorldPosition, 1.0);

    // RT1: 世界空间法线
    vec3 N;
    if (useNormalMap > 0) {
        // 从法线贴图读取切线空间法线，转换到世界空间
        N = texture(normalTex, vUV).rgb;
        N = N * 2.0 - 1.0;            // [0,1] -> [-1,1]
        N = normalize(vTBN * N);       // 切线空间 -> 世界空间
    } else {
        N = normalize(vNormal);
    }
    gNormal = vec4(N, 0.0);

    // RT2: 反照率（sRGB空间采样后自动线性化）+ 金属度
    vec3 albedo = texture(albedoTex, vUV).rgb;
    float metallic = texture(metallicTex, vUV).b;   // 通常存储在蓝色通道
    gAlbedo = vec4(albedo, metallic);

    // RT3: 粗糙度 + AO + 自发光标志
    float roughness = texture(roughnessTex, vUV).r;  // 通常存储在红色通道
    float ao = 1.0;                                   // 如果有AO贴图可以在这里采样
    float emissive = 0.0;                              // 自发光标志
    gParam = vec4(roughness, ao, emissive, 1.0);
}
