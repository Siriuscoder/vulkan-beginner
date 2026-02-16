#version 450

layout(location = 0) in vec3 vWNormal;
layout(location = 0) out vec4 outColor;

const vec3 lightDir = vec3(2.0, -3.0, 5.0);
const vec3 baseColor = vec3(0.3, 0.6, 0.9);
const vec3 ambientColor = vec3(0.1);

void main() {
    //outColor = gl_FrontFacing ? vec4(0,1,0,1) : vec4(1,0,0,1);

    vec3 L = normalize(lightDir);
    vec3 N = normalize(vWNormal);
    float NdotL = max(dot(N, L), 0.0);

    vec3 color = ambientColor + (baseColor * NdotL);

    outColor = vec4(color, 1.0);
}
