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
uniform int numSpotLights;

layout(std430, binding=10) buffer point_light_list
{
    vec4 lights[];
};

layout (location = 0) out vec4 finalColor;

float attenuate(in const float distSquared, in const float radiusSquared) {
    float frac = distSquared / radiusSquared;
    float atten = max(0, 1 - frac * frac);
    return (atten * atten) / (1.0 + distSquared);
}

void main() {
    vec3 n = normalize(normalInEyeSpace);
    vec3 e = normalize(-vertexInEyeSpace);
    
    vec3 diffuseColor = kd;
    if (textured) {
        diffuseColor *= texture(diffuseTex, texCoord).xyz;
    }

    vec3 outColor = ke + ka*ambientLight;
    
    for (int i = 0; i < numDirectionalLights; ++i) {
        vec3 lightDir   = lights[3 * i + 0].xyz;
        vec3 lightColor = lights[3 * i + 1].xyz;
        vec3 l = normalize(-lightDir);
        vec3 h = normalize(l + e);
        outColor += lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            outColor += lightColor * ks * pow(max(dot(h, n), 0.0), 4*specular);
    }
    
    int numLights = numDirectionalLights;
    for (int i = 0; i < numPointLights; ++i) {
        vec4 lightPR    = lights[3 * (numLights + i) + 0];
        vec3 lightColor = lights[3 * (numLights + i) + 1].xyz;
        vec3 lightPos   = lightPR.xyz;
        float lightRadiusSquared = lightPR.w;

        vec3 vertToLight = lightPos - vertexInEyeSpace;
        vec3 l = normalize(vertToLight);
        vec3 h = normalize(l + e);
        float d2 = dot(vertToLight, vertToLight);
        float atten = attenuate(d2, lightRadiusSquared);
        
        outColor += atten * lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            outColor += atten * lightColor * ks * pow(max(dot(h, n), 0.0), 4*specular);
    }
    
    numLights += numPointLights;
    for (int i = 0; i < numSpotLights; ++i) {
        vec4 data                = lights[3 * (numLights + i) + 0];
        vec3 lightPos            = data.xyz;
        float lightRadiusSquared = data.w;
        
        data                     = lights[3 * (numLights + i) + 1];
        vec3 lightColor          = data.xyz;
        float innerCutoff        = data.w;
        
        data                     = lights[3 * (numLights + i) + 2];
        vec3 lightDir            = data.xyz;
        float outterCutoff       = data.w;

        vec3 vertToLight = lightPos - vertexInEyeSpace;
        vec3 l = normalize(vertToLight);
        vec3 h = normalize(l + e);
        
        float theta = dot(-l, lightDir);
        if (theta > outterCutoff) {
            float epsilon = innerCutoff - outterCutoff;
            float intensity = clamp((theta - outterCutoff) / epsilon, 0.0, 1.0);
            float d2 = dot(vertToLight, vertToLight);
            float atten = intensity * attenuate(d2, lightRadiusSquared);

            outColor += atten * lightColor * diffuseColor * max(0.0, dot(l, n));
            if (dot(l, n) > EPSILON)
                outColor += atten * lightColor * ks * pow(max(dot(h, n), 0.0), 4*specular);
        }
    }
    
    finalColor = vec4(outColor, 1.0);
}
