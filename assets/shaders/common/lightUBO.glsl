// Shared light UBO layout - must match C++ struct exactly (std140)
// std140 rules: vec4=16B align, float=4B align, struct aligned to largest member

#define MAX_POINT_LIGHTS 8

struct GPUDirectionalLight {
	vec4 direction;          // xyz = direction           (offset  0, size 16)
	vec4 color;              // xyz = color, w = intensity (offset 16, size 16)
	vec4 specularPad;        // x = specularIntensity      (offset 32, size 16)
};
// total per struct: 48 bytes

struct GPUPointLight {
	vec4 position;           // xyz = position             (offset  0, size 16)
	vec4 color;              // xyz = color                (offset 16, size 16)
   vec4 attenuation;        // x=specular, y=k2, z=k1, w=kc (offset 32, size 16)
	vec4 params;             // x=effectiveRange           (offset 48, size 16)
};
// total per struct: 64 bytes

layout(std140, binding = 0) uniform LightBlock {
	GPUDirectionalLight dirLight;                      // offset   0, size  48
  GPUPointLight       pointLights[MAX_POINT_LIGHTS]; // offset  48, size 512
	int                 numPointLights;                // offset 560, size   4
	// pad to 16-byte boundary: 12 bytes padding       // total  576
};
