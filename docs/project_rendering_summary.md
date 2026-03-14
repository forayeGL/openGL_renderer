# 项目实现思路详细总结 & 面试知识点清单

> 本文档基于项目源码分析所得，汇总了项目中以下功能的实现思路：模板测试，颜色混合，多pass渲染，实例渲染，gamma矫正，法线贴图 TBN，视差贴图，Shadowmap（自适应 bias、采样边缘处理）、PCF/PCSS、CSM、点光源阴影、MSAA、HDR、Bloom、PBR、IBL；并对代码架构（object、mesh、material、renderer、renderpipeline、shader、着色器代码、UBO、UI 系统）做出总结，最后给出面试知识点清单。

> 说明：文中代码片段和结构名称均对应项目源码中的类与文件路径，便于阅读与对照。

---

## 一、各渲染技术实现思路

### 1. 模板测试 (Stencil Test)

实现架构：状态数据完全封装在 `Material` 基类中，`Renderer::setStencilState()` 负责提交 GL 状态。

- `Material` 字段：
  - `mStencilTest` → glEnable/Disable(GL_STENCIL_TEST)
  - `mSFail/mZFail/mZPass` → glStencilOp(...)
  - `mStencilMask` → glStencilMask(...)
  - `mStencilFunc` + `mStencilRef` + `mStencilFuncMask` → glStencilFunc(...)

典型用法（描边效果）：
- Pass 1：正常材质，`mStencilFunc=GL_ALWAYS`, `mZPass=GL_REPLACE`, `mStencilRef=1` → 把物体轮廓写入模板缓冲
- Pass 2：白色材质 + 略微放大，`mStencilFunc=GL_NOTEQUAL, ref=1` → 只在模板值≠1处绘制，形成外轮廓


### 2. 颜色混合 (Color Blending)

实现架构：`Material` 基类持有混合参数，`Renderer` 在 `render()` 入口对所有 Mesh 做透明/不透明分桶，渲染时先不透明再透明。

核心点：
- 在 `Renderer::projectObject()` 中根据 `material->mBlend` 将 Mesh 分入 `mOpacityObjects` 或 `mTransparentObjects`。
- 对透明对象按相机深度从远到近排序（画家算法）。
- `Renderer::setBlendState()` 根据 `material->mSFactor/mDFactor` 调用 `glBlendFunc`。


### 3. 多Pass渲染 (Multi-Pass Rendering)

实现核心：`IRenderPipeline` 抽象接口 + `DefaultRenderPipeline::execute()` 按顺序串联各 Pass，通过 `Framebuffer` 在 Pass 之间传递纹理数据。

流水线（`DefaultRenderPipeline::execute`）典型顺序：
1. Shadow Pass(es)：方向光/点光阴影渲染到各自的 Shadow FBO
2. Main Scene → MSAA HDR FBO（多采样浮点颜色缓冲）
3. MSAA Resolve：`glBlitFramebuffer` 把 MSAA FBO 解析到普通 HDR FBO
4. Bloom 后处理（多级下采样/上采样/合并）
5. Post Scene → 屏幕（最终 Tone Mapping 与 Gamma 校正）

设计要点：`RenderContext` 作为数据令牌，将 Scene/Camera/Light/设置与 Pipeline 解耦，方便替换 Pipeline。


### 4. 实例渲染 (Instanced Rendering)

实现架构：`InstancedMesh` 继承 `Mesh`，持有 `mInstanceMatrices` 和对应 `mMatrixVbo`。

关键技术：
- 在 VAO 中为 `mat4` 的每一列设置 attribute（4 个 vec4）并设置 `glVertexAttribDivisor(..., 1)`。
- 绘制时使用 `glDrawElementsInstanced(..., instanceCount)`。
- 提供 `updateMatrices()` 与 `sortMatrices()`（按深度排序）以支持动态实例和透明实例处理。


### 5. Gamma 校正 (Gamma Correction)

实现位置：`screen.frag`（最终全屏 Quad 的 Fragment Shader），在 Tone Mapping 之后执行。

流程：
1. 从 HDR 纹理采样得到线性空间颜色
2. Tone Mapping（如 Exposure 或 Reinhard）将 HDR 压缩到 LDR 范围
3. Gamma 编码：`color = pow(color, vec3(1.0/2.2))` → 将线性转换为 sRGB 展示

说明：场景光照计算在线性空间进行，贴图若为 sRGB 则应在采样时解码或者让 GL 自动解码（`GL_SRGB`）。


### 6. 法线贴图 TBN (Normal Map TBN)

Vertex Shader（如 `phongNormal.vert`）构建 TBN：
- 顶点属性有 `aNormal`、`aTangent`。
- `tangent = normalize(mat3(modelMatrix) * aTangent)`
- `bitangent = normalize(cross(normal, tangent))`
- `tbn = mat3(tangent, bitangent, normal)`（用于在 Fragment 中将切线空间法线映射到世界空间）

Fragment Shader：
- `normalN = texture(normalMap, uv).rgb * 2.0 - 1.0`
- `normalN = normalize(tbn * normalN)`

注意：若模型存在非均匀缩放，使用 `normalMatrix = transpose(inverse(mat3(modelMatrix)))` 来变换法线。


### 7. 视差贴图 (Parallax Mapping)

实现（`phongParallax.frag`）：基于“分层高度图法”（steep parallax / parallax occlusion）：
- 把视线方向变换到切线空间：`viewDir = normalize(transpose(tbn) * viewDir)`
- 以 `layerNum` 分层，计算每层的 UV 偏移 `deltaUV = viewDir.xy / viewDir.z * heightScale / layerNum`
- 沿层迭代直到样本深度小于当前累积深度
- 用最后两层做线性插值，得到更平滑的 `uvReal`

之后用 `uvReal` 来采样 diffuse、normal 等贴图。


### 8. Shadow Map：自适应 Bias、采样边缘处理、PCF、PCSS

自适应 Bias（着色器）：
```
float getDirBias(vec3 N, vec3 lightDir) {
    return max(shadowBias * (1.0 - dot(normalize(N), normalize(lightDir))), 0.0005);
}
```
解释：当法线与光方向夹角变大（掠射）时，bias 增大以减少阴影 acne。

边缘处理：超出光源投影范围的片元直接返回不在阴影中（例如 `if (depth > 1.0) return 0.0;`）。

PCF（Percentage Closer Filtering）：
- 使用泊松圆盘（Poisson disk）采样集并以当前片元 UV 为随机种子旋转，进行多次深度比较，返回阴影权重的均值，从而软化阴影边缘。

PCSS（Percentage Closer Soft Shadows）思路：
- 先做 blocker search（寻找遮挡者并计算平均遮挡深度）
- 估算 penumbra 半径：`penumbra = (receiverDist - avgBlockerDist) * lightSize / avgBlockerDist`
- 用该 penumbra 作为 PCF 采样半径，近处阴影清晰、远处阴影模糊

着色器参数：`mDiskTightness` 控制泊松盘分布紧密度，`mLightSize` 控制光源面积近似。


### 9. CSM (Cascaded Shadow Maps)

`DirectionalLightCSMShadow` 实现了分层阴影，常用关键点：
- 分割策略：对数 / 混合对数-线性分割，项目使用近似对数：`layer = near * pow(far/near, i / layerCount)`，更偏重近距离精度。
- 每级计算方法：
  1. 使用相机在该级 near/far 区间构建投影矩阵并求出视锥的 8 个角点（世界空间）
  2. 将角点变换到 light view space，求 AABB 范围
  3. 扩展 Z 范围以确保覆盖（示例中 `maxZ *= 10`）
  4. 用 `glm::ortho(minX, maxX, minY, maxY, -maxZ, -minZ)` 构造正交投影，得到 `lightMatrix`
- Shader 使用 `sampler2DArray` 或多纹理层存储各层深度并按片元相机深度选择当前层进行采样。


### 10. 点光源阴影 (Point Light Shadow)

实现：立方体贴图（Cubemap）深度渲染
- `PointLightShadow::getLightMatrices` 返回 6 个 `proj * lookAt` 矩阵（FOV=90°），对应 `+X, -X, +Y, -Y, +Z, -Z`。
- 深度通常以线性距离 / far 归一化以便存储到纹理。
- Fragment 端：用 `normalize(worldPos - lightPos)` 作为采样方向，从 `samplerCube` 采样并做 PCF（3D 泊松球采样）。


### 11. MSAA (Multi-Sample Anti-Aliasing)

实现细节：
- 创建 MSAA HDR FBO：颜色附件使用 `GL_TEXTURE_2D_MULTISAMPLE`（`GL_RGB16F`），深度模板也使用多采样格式。
- Resolve：使用 `glBlitFramebuffer` 从读帧（MSAA）到写帧（单采样 HDR）进行颜色缓冲解析（GPU 自动对样本进行平均）。
- 注意：多采样纹理通常不可直接作为采样器在着色器中使用，需要先 Blit 到单采样纹理。


### 12. HDR (High Dynamic Range)

全流程：
- 场景渲染到 `GL_RGB16F` 浮点 FBO，允许亮度 > 1.0
- 保持 HDR 格式通过所有后处理（Bloom 等）
- 在最终屏幕渲染前做 Tone Mapping（如 Exposure 或 Reinhard），然后做 Gamma 编码输出到屏幕

Tone Mapping 示例（项目）:
- Exposure: `1.0 - exp(-hdrColor * exposure)`
- Reinhard: `hdrColor / (hdrColor + 1.0)`


### 13. Bloom

实现：`Bloom` 类维护一套下采样与上采样的 FBO 链

流程：
1. 把原 HDR 图拷贝到 `mOriginFbo`（备份）
2. `extractBright`：阈值过滤，保留高亮像素，输出到第一层 downsample
3. `downSample` 链：逐级分辨率减半（每级用 `glBlitFramebuffer`）
4. `upSample` 链：从最小分辨率逐级上采样并与更高分辨率混合（`mUpSampleShader`）
5. `merge`：用 `mMergeShader` 将 bloom 结果与原始图合并（`origin + bloom * intensity`）

参数：`mThreshold`, `mBloomRadius`, `mBloomAttenuation`, `mBloomIntensity`。


### 14. PBR (Physically Based Rendering)

`pbr.frag` 实现 Cook-Torrance BRDF：
- NDF（GGX）
- Geometry（Smith + Schlick-GGX）
- Fresnel（Schlick 近似）

实现要点：
- `F0 = mix(vec3(0.04), albedo, metallic)`
- 漫反射系数 `kd = (1 - F) * (1 - metallic)`
- specular = `(D * G * F) / (4 * NdotL * NdotV)`
- IBL diffuse：`irradiance = texture(irradianceMap, N)` → `ambient = irradiance * albedo * kd`

材质贴图：`albedoTex`, `roughnessTex`, `metallicTex`, `normalTex`。


### 15. IBL (Image-Based Lighting)

实现要点：
- 使用预计算的 `irradianceMap`（Diffuse IBL）来近似来自环境的漫反射光
- Specular IBL 可扩展：需要 prefiltered env map（按 roughness 的 mipmap）+ BRDF LUT（split-sum approximation）
- 在 PBR shader 中，将 IBL 的漫反射与点光源的直接光叠加


## 二、代码架构总结

```
RendererApp                          ← 应用入口，管理生命周期
  ├─ IRenderPipeline (接口)
  │    └─ DefaultRenderPipeline      ← 默认前向渲染管线
  │         ├─ Renderer              ← 核心渲染器（状态机 + 绘制调度）
  │         │    └─ UBOManager       ← 管理3个 UBO（灯光/阴影/渲染设置）
  │         ├─ Bloom                 ← 独立后处理模块
  │         ├─ mFboMulti (MSAA HDR)
  │         └─ mFboResolve (HDR)
  │
  ├─ Scene (场景树，Object 层级)
  │    └─ Object → Mesh → InstancedMesh
  │                  ├─ Geometry (VAO/VBO/EBO)
  │                  └─ Material (基类)
  │                       ├─ PhongMaterial / PhongShadowMaterial / PhongCSMShadowMaterial
  │                       ├─ PhongNormalMaterial / PhongParallaxMaterial
  │                       ├─ PhongPointShadowMaterial
  │                       ├─ PbrMaterial
  │                       ├─ ScreenMaterial (后处理)
  │                       └─ CubeMaterial (天空盒)
  │
  ├─ ShaderManager (单例，vert+frag 路径为key，缓存 unique_ptr<Shader>)
  │    └─ Shader (编译/链接/uniform helper，支持 #include)
  │
  ├─ Light 体系
  │    ├─ DirectionalLight → DirectionalLightShadow / DirectionalLightCSMShadow
  │    └─ PointLight       → PointLightShadow
  │
  ├─ RenderContext (帧数据令牌：scene/camera/lights/settings)
  │
  └─ GuiSystem
       ├─ IGuiPanel (接口：onRender / onInit)
       ├─ ScenePanel     ← 场景对象管理
       ├─ LightingPanel  ← 灯光参数
       └─ RenderingPanel ← 渲染模式/曝光/Bloom参数
```

UBO 绑定点约定：

- Binding 0: `LightBlock`（方向光 + 点光数组 + 数量）
- Binding 1: `ShadowBlock`（lightMatrix + bias + pcf 参数 + far 值数组）
- Binding 2: `RenderSettingsBlock`（ambientColor + cameraPosition + renderMode）


## 三、面试知识点清单

### 渲染管线基础

- OpenGL 渲染管线各阶段（顶点→图元→光栅化→Fragment→输出合并）
- VAO/VBO/EBO 的区别与作用
- `glVertexAttribDivisor` 实现实例渲染的原理
- 深度测试的函数类型（`GL_LESS/GL_LEQUAL/GL_ALWAYS`）及用途
- 深度写入 `glDepthMask(GL_FALSE)` 在透明物体渲染中的作用
- 面剔除（Face Culling）与正面定义（`GL_CCW/GL_CW`）


### 阴影技术

- Shadow Map 基本原理：光源视角深度渲染 + 片元比较
- Shadow Acne 成因与解决（bias、深度偏移、PCF）
- Peter Panning 问题（bias 过大导致阴影与物体分离）
- PCF：多点深度比较平均化以软化阴影边缘
- 泊松圆盘采样 vs 均匀采样
- PCSS：blocker search + penumbra 估算 + 可变 PCF 半径
- CSM：对数分割与每级正交投影矩阵的计算
- 点光源阴影（Cubemap）实现细节


### 光照模型

- Phong vs Blinn-Phong
- PBR 能量守恒（kd + ks = 1、kd *= (1 - metallic)）
- Cook-Torrance BRDF 组成（D, G, F）
- GGX NDF 与 roughness 的关系
- Schlick-GGX Geometry
- Fresnel Schlick 近似
- 金属度工作流（F0 混合）


### 法线贴图与视差贴图

- TBN 构建（三向量：T、B、N）
- `normalMatrix = transpose(inverse(mat3(modelMatrix)))` 的使用场景
- Parallax / POM 实现原理


### HDR & Tone Mapping & Gamma

- 使用 `GL_RGB16F` 的原因
- Reinhard / Exposure Tone Mapping 差异
- Gamma 校正顺序（在 Tone Mapping 之后）
- 线性工作流的重要性


### MSAA

- `GL_TEXTURE_2D_MULTISAMPLE` 与采样原理
- MSAA 对几何边缘有效但不消除 shader 内混叠
- `glBlitFramebuffer` Resolve 的作用


### Bloom

- 高光提取 → 下采样 → 上采样 → 合并
- 下采样链的优势（性能、频域分离）
- 阈值在 HDR 空间的含义


### IBL

- Diffuse IBL（irradiance cubemap）
- Specular IBL（prefiltered env map + BRDF LUT）
- Split-sum approximation 的原因


### 代码架构设计

- UBO（Uniform Buffer Object）优点：数据共享、减少重复 set uniform、std140 对齐要求
- std140 对齐规则要点（vec4 = 16B，mat4 = 64B，float 数组每元素 16B）
- ShaderManager 的缓存策略（以 shader 路径作为 key）
- Material 驱动的 GL 状态机设计（深度、模板、混合、剔除）
- RenderContext 的 Token 模式解耦 Pipeline 与 App
- IRenderPipeline 接口支持管线替换
- 透明物体深度排序与正确 Blend 顺序
- FBO / Viewport 状态保存与恢复（shadow pass 前后）
- GUI 模块化（`IGuiPanel`）与 ImGui 生命周期管理（NewFrame/Render）


### 常见面试问题

- Shadow Map 分辨率不足的解决策略（CSM，PCF，增加分辨率）
- CSM 对数分割的优点
- 为什么需要 MSAA Resolve
- PBR 中 roughness 等极端值的数值稳定性处理（防止除 0）
- IBL 为何拆分 diffuse/specular


---

## 参考（源码位置示例）

- `application/rendererApp.h|cpp`
- `glframework/renderer/default_render_pipeline.*`
- `glframework/renderer/renderer.*`
- `glframework/renderer/ubo_manager.*`
- `glframework/light/shadow/*`（`directionalLightShadow`, `directionalLightCSMShadow`, `pointLightShadow`）
- `glframework/framebuffer/framebuffer.*`
- `assets/shaders/advanced/*`（`phongShadow.*`, `phongCSMShadow.*`, `phongNormal.*`, `phongParallax.*`, `pbr/*`）

---

> 说明：本文档根据仓库源码摘录与归纳，保留了实现要点与面试相关的理论知识点，便于阅读复盘与面试准备。若需要将某一部分进一步展开（如给出完整着色器关键函数的数学推导、PCSS 算法具体实现或 CSM 的边界稳定化策略），可以在此基础上继续扩展。
