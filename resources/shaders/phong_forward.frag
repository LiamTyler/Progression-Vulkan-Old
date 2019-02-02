#version 430 core

#define EPSILON 0.000001

in vec3 vertexInEyeSpace;
in vec3 normalInEyeSpace;
in vec2 texCoord;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform vec3 ke;
uniform float specular;

uniform bool textured;
uniform sampler2D diffuseTex;

uniform vec3 ambientLight;
uniform int numDirectionalLights;
uniform int numPointLights;

layout(std430, binding=10) buffer point_light_list
{
    vec4 lights[];
};

const float k1 = .22;
const float k2 = .2;

layout (location = 0) out vec4 finalColor;

void main() {
    vec3 n = normalize(normalInEyeSpace);
    vec3 e = normalize(-vertexInEyeSpace);
    
    vec3 diffuseColor = kd;
    if (textured) {
        diffuseColor *= texture(diffuseTex, texCoord).xyz;
    }

    vec3 outColor = ke + ka*ambientLight;
    
    for (int i = 0; i < numDirectionalLights; ++i) {
        vec3 lightDir   = lights[2 * i + 0].xyz;
        vec3 lightColor = lights[2 * i + 1].xyz;
        vec3 l = normalize(-lightDir);
        vec3 h = normalize(l + e);
        outColor += lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            outColor += lightColor * ks * pow(max(dot(h, n), 0.0), 4*specular);
    }
    
    const float I = 7.57;
    const float R = 20.0;
    
    for (int i = 0; i < numPointLights; ++i) {
        vec4 lightPR    = lights[2 * (numDirectionalLights + i) + 0];
        vec3 lightColor = lights[2 * (numDirectionalLights + i) + 1].xyz;
        vec3 lightPos   = lightPR.xyz;
        float lightRadiusSquared = lightPR.w;

        vec3 l = normalize(lightPos - vertexInEyeSpace);
        vec3 h = normalize(l + e);
        vec3 vertToLight = lightPos - vertexInEyeSpace;
        float d2 = dot(vertToLight, vertToLight);
        float frac = d2 / lightRadiusSquared;
        float atten = max(0, 1 - frac * frac);
        atten = (atten * atten) / (1 + d2);

        outColor += atten * lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            outColor += atten * lightColor * ks * pow(max(dot(h, n), 0.0), 4*specular);
    }
    
    finalColor = vec4(outColor, 1.0);
}
