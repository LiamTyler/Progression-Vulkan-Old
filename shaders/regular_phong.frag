#version 330 core

in vec3 vertexInEyeSpace;
in vec3 normalInEyeSpace;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float specular;

uniform vec3 Ia;
uniform vec3 Id;
uniform vec3 Is;

uniform vec3 lightInEyeSpace;

out vec4 finalColor;

void main() {
    vec3 n = normalize(normalInEyeSpace);
    vec3 l = normalize(-lightInEyeSpace);
    vec3 e = normalize(-vertexInEyeSpace);
    vec3 h = normalize(l + e);

    vec3 outColor = vec3(0, 0, 0);
    outColor += Ia * ka;
    outColor += Id * kd * max(0.0, dot(l, n));
    outColor += Is * ks * pow(max(dot(h, n), 0.0), specular);

    finalColor.rgb = outColor;
    finalColor.a   = 1.0;
}
