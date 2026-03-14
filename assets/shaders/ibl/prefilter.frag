#version 460 core
out vec4 FragColor;
in  vec3 localPos;

uniform samplerCube environmentMap;
uniform float       roughness;

const float PI           = 3.14159265359;
const uint  SAMPLE_COUNT = 1024u;

// ─── Low-discrepancy sequence (Hammersley) ───────────────────────────────────

float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0Fu) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), radicalInverse_VdC(i));
}

// ─── GGX importance sampling ─────────────────────────────────────────────────

// Samples a half-vector H biased toward the GGX specular lobe around N.
vec3 importanceSampleGGX(vec2 Xi, vec3 N, float r) {
    float a = r * r;
    float phi      = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // Spherical → tangent-space cartesian
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    // Tbn: 
    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent   = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

// GGX normal distribution function D(h)
float distributionGGX(float NdotH, float r) {
    float a  = r * r;
    float a2 = a * a;
    float d  = NdotH * NdotH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

// ─── Main ────────────────────────────────────────────────────────────────────

void main() {
    // For the split-sum approximation we assume V = R = N, which avoids
    // storing a full 4D function at the cost of slightly incorrect off-axis
    // reflections at grazing angles.
    vec3 N = normalize(localPos);
    vec3 V = N;

    // Texel solid angle of the source env map (used for mip selection)
    float envFaceWidth = float(textureSize(environmentMap, 0).x);
    float saTexel = 4.0 * PI / (6.0 * envFaceWidth * envFaceWidth);

    vec3  prefilteredColor = vec3(0.0);
    float totalWeight      = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H  = importanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            float NdotH = max(dot(N, H), 0.0);
            float VdotH = max(dot(V, H), 0.0);

            // PDF of this GGX sample: p(h) = D(h)*NdotH / (4*VdotH)
            float D   = distributionGGX(NdotH, roughness);
            float pdf = (D * NdotH / (4.0 * VdotH)) + 0.0001;

            // Solid angle covered by this sample vs. one texel — select mip
            // level so that each sample fetches roughly one env-map texel,
            // eliminating high-frequency noise (fireflies).
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            prefilteredColor += textureLod(environmentMap, L, mipLevel).rgb * NdotL;
            totalWeight      += NdotL;
        }
    }

    prefilteredColor /= max(totalWeight, 0.0001);
    FragColor = vec4(prefilteredColor, 1.0);
}
