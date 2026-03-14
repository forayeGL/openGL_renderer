// Shared render settings UBO - must match C++ struct exactly (std140)

layout(std140, binding = 2) uniform RenderSettingsBlock {
	vec4  ambientColor;       // xyz = color                (offset  0, size 16)
	vec4  uCameraPosition;    // xyz = position             (offset 16, size 16)
	vec4  renderParams;       // x = opacity, y = float(renderMode), z = float(shadowType) (offset 32, size 16)
};
// total: 48 bytes

// Convenience accessors
#define uOpacity    renderParams.x
#define renderMode  int(renderParams.y)
#define shadowType  int(renderParams.z)
