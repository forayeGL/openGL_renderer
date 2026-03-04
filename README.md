# OpenGL Renderer

Modern OpenGL rendering framework and demo application implemented in C++17 and GLSL 4.6. The project demonstrates a modular rendering pipeline with PBR, multiple shadow algorithms, HDR post-processing, instanced rendering and an ImGui-driven runtime UI for live tuning.

This repository is intended as a learning / prototyping engine and a base for extending rendering experiments.

## Highlights

- Physically Based Rendering (PBR) with Cook-Torrance BRDF
- Multiple shadow techniques: ShadowMap, PCF, PCSS, Point (cube) shadows, and CSM
- HDR rendering pipeline with MSAA resolve and Bloom (mip-chain down/up sampling)
- Uniform Buffer Objects (UBOs) for Lights / Shadow / Render settings (std140-safe)
- Instanced rendering, scene graph with hierarchical transforms
- Shader manager with `#include` preprocessing for shared GLSL modules
- Dear ImGui integration for runtime parameter editing

## Features

- Phong and PBR materials
- Normal mapping, Parallax mapping, and advanced material workflows
- Environment lighting via HDR `.exr` skybox / IBL
- Post-processing: exposure, bloom, tone mapping
- Runtime controls: add/remove objects and point lights, tweak shadow bias/PCF radius, switch render modes

## Requirements

- CMake >= 3.12
- Ninja (recommended) or other generator supported by CMake
- C++17-capable compiler (MSVC/Clang/GCC)
- OpenGL 4.6 capable GPU and drivers

Third-party libraries used (vendored or required): GLFW, GLAD, GLM, Dear ImGui, stb_image, OpenEXR

## Build

Example (Unix/macOS with Ninja):

```bash
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja
./openGL_renderer
```

Windows (Developer PowerShell, Ninja):

```powershell
mkdir build; cd build
cmake -G "Ninja" -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
.\openGL_renderer.exe
```

Note: shaders and assets are loaded from `assets/` relative to the working directory. Run the binary from the repository root or configure working directory accordingly.

## Project layout

- `application/` — application entry, scene setup, ImGui UI
- `glframework/` — renderer core, materials, shaders, meshes, textures, UBO manager
- `assets/shaders/` — GLSL shader files and shared includes (UBO declarations)
- `assets/textures/` — textures and HDR environment maps

Key modules: `RenderPipeline`, `Renderer`, `UBOManager`, `ShaderManager`, `Material` subclasses

## Extending & Contributing

- Add new `Material` subclass and shaders; register uniforms in `applyUniforms()` for per-object data. Use UBOs for global per-frame data.
- When changing UBOs, strictly follow `std140` layout rules: pack into `vec4/ivec4/mat4` to avoid C++/GLSL mismatches.
- Follow coding style in `glframework/` and keep shader includes modular.

If you'd like, I can add a CONTRIBUTING.md with development workflow, or a GitHub Actions CI file to build the project automatically.

## License

This project is licensed under the MIT License - see `LICENSE` for details.

