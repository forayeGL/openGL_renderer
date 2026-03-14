#version 460 core

in vec3 worldPosition;

uniform float far;
uniform vec3 lightPosition;

//不需要输出任何颜色
//用于绘制ShadowMap的FBO只有深度附件没有颜色附件
void main(){
	float lightDis = length(worldPosition - lightPosition);
	lightDis = lightDis / (1.414 * far);

	gl_FragDepth = lightDis;
}