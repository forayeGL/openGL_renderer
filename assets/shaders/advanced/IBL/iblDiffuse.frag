#version 460 core
out vec4 FragColor;
in vec3 uvw;

uniform sampler2D envMap;
const float PI = 3.1415926535897932384626433832795;

// 三维向量转换为二维纹理坐标，其中uvwN是单位向量，
// 表示从球心指向表面某点的方向，通过计算该方向的极角和方位角来得到对应的纹理坐标u和v。
// phi是极角，theta是方位角，u和v分别表示纹理坐标的水平和垂直分量。
// 这种转换方法适用于环境贴图（environment mapping），可以将三维空间中的方向映射到二维纹理上，从而实现基于环境贴图的照明效果。
// 例如，在基于环境贴图的照明中，我们需要根据表面法线方向采样环境贴图来计算漫反射和镜面反射的贡献。
// 通过将表面法线方向转换为纹理坐标，我们可以从环境贴图中获取对应的颜色值，从而实现更真实的照明效果。
vec2 uvwToUv(vec3 uvwN){
    float phi = asin(uvwN.y);
    float theta = atan(uvwN.z, uvwN.x);
    float u = theta / (2.0 * PI) + 0.5;
    float v = phi / PI + 0.5;
    return vec2(u, v);
}

void main(){
    vec3 uvwN = normalize(uvw);
    vec3 irradiance = vec3(0.0);
    // 构建一个局部坐标系，uvwN作为法线向量，up和right分别是垂直于uvwN的两个向量
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 right = normalize(cross(up, uvwN));
    up = normalize(cross(uvwN, right));
    float sampleDelta = 0.025;
    // 在半球上采样，计算每个采样点的贡献
    for(float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta){
        for(float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta){
            // 将球面坐标转换为局部笛卡尔坐标系中的方向向量
            vec3 sampleDir = vec3( sin(theta)*cos(phi),  cos(theta),  sin(theta)*sin(phi) );
            // 将局部方向向量转换为世界坐标系中的方向向量
            vec3 sampleVec = right * sampleDir.x + up * sampleDir.y + uvwN * sampleDir.z;
            // 计算采样点的权重，cosine权重根据采样点与法线的夹角计算，sampleDelta是采样间隔
            vec2 uv = uvwToUv(sampleVec);
            vec3 Li = texture(envMap, uv).rgb;
            irradiance += Li*cos(theta)*sin(theta);
            
            }
        }
        irradiance =PI*irradiance*1.0*sampleDelta*sampleDelta;
        FragColor = vec4(irradiance, 1.0);
    }