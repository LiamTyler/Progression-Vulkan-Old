#version 430 core

#define EPSILON 0.000001

in vec3 fragPosInWorldSpace;
in vec3 normalInWorldSpace;
in vec4 fragPosInLightSpace;
in vec2 texCoord;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform vec3 ke;
uniform float specular;

uniform vec3 cameraPos;
uniform bool textured;
uniform sampler2D diffuseTex;
uniform sampler2D depthTex;

uniform int numDirectionalLights;
uniform int numPointLights;


layout(std430, binding=10) buffer point_light_list
{
    vec4 lights[];
};


uniform vec3 lightDir;

uniform float bloomThreshold;

layout (location = 0) out vec4 finalColor;

float ShadowCalculation() {
    vec3 ndc = fragPosInLightSpace.xyz / fragPosInLightSpace.w;
    vec3 projCoords = 0.5 * ndc + vec3(0.5);
    float currentDepth = projCoords.z;
    float shadowDepth = texture(depthTex, projCoords.xy).r;
    return currentDepth - 0.005 > shadowDepth ? 0.0 : 1.0;
}

void main() {
    vec3 n = normalize(normalInWorldSpace);
    vec3 e = normalize(cameraPos - fragPosInWorldSpace);
    
    vec3 diffuseColor = kd;
    if (textured) {
        diffuseColor *= texture(diffuseTex, texCoord).xyz;
    }

    vec3 outColor = ke;
    
    vec3 lightColor = vec3(1, 1, 1);
    vec3 l = normalize(-lightDir);
    //vec3 l = normalize(-lights[0].xyz);
    vec3 h = normalize(l + e);
    outColor += lightColor * ka;
    vec3 shadowColor = lightColor * diffuseColor * max(0.0, dot(l, n));
    if (dot(l, n) > EPSILON)
        shadowColor += lightColor * ks * pow(max(dot(h, n), 0.0), 4*specular);
    
    float shadow = ShadowCalculation();
    outColor += shadow * shadowColor;
    /*
    for (int i = 0; i < numDirectionalLights; ++i) {
        vec3 lightDir   = lights[2 * i + 0].xyz;
        vec3 lightColor = lights[2 * i + 1].xyz;
        vec3 l = normalize(-lightDir);
        vec3 h = normalize(l + e);
        outColor += lightColor * ka;
        outColor += lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            outColor += lightColor * ks * pow(max(dot(h, n), 0.0), 4*specular);
    }
        
    for (int i = 0; i < numPointLights; ++i) {
        vec3 lightPos   = lights[2 * (numDirectionalLights + i) + 0].xyz;
        vec3 lightColor = lights[2 * (numDirectionalLights + i) + 1].xyz;

        vec3 l = normalize(lightPos - vertexInEyeSpace);
        vec3 h = normalize(l + e);
        float attenuation = 1.0 / pow(length(lightPos - vertexInEyeSpace), 2.0);
        outColor += lightColor * ka;
        outColor += attenuation * lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            outColor += attenuation * lightColor * ks * pow(max(dot(h, n), 0.0), 4*specular);
    }
    */
    
    finalColor = vec4(outColor, 1.0);
}
