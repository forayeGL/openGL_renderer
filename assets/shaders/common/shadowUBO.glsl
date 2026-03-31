// Shared shadow UBO layout - must match C++ struct exactly (std140)
// float arrays in std140: each element rounds up to vec4 (16 bytes)!
// So we pack shadow far values into vec4s instead.

#define MAX_POINT_SHADOW 8
#define MAX_CSM_CASCADES 8

layout(std140, binding = 1) uniform ShadowBlock {
	mat4  shadowLightMatrix;      // offset   0, size 64
	mat4  shadowLightViewMatrix;  // offset  64, size 64
	vec4  shadowParams1;          // x=bias, y=pcfRadius, z=diskTightness, w=lightSize  (offset 128)
	vec4  shadowParams2;          // x=nearPlane, y=frustum, z=unused, w=unused          (offset 144)
	vec4  pointShadowFar01;       // far[0..3]                                           (offset 160)
	vec4  pointShadowFar23;       // far[4..7]                                           (offset 176)
   ivec4 shadowFlags;            // x=hasDirShadow, y=numPointShadows, z=csmCascadeCount (offset 192)
	mat4  csmLightMatrices[MAX_CSM_CASCADES]; //                                        (offset 208)
	vec4  csmSplits01;            // split[0..3]                                          (offset 720)
	vec4  csmSplits23;            // split[4..7]                                          (offset 736)
};
// total: 752 bytes (aligned to 16 = 752)
