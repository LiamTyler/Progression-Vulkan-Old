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

uniform int numDirectionalLights;
uniform int numPointLights;

layout(std430, binding=10) buffer point_light_list
{
    vec4 lights[];
};

uniform float bloomThreshold;

layout (location = 0) out vec4 finalColor;

void main() {
    vec3 n = normalize(normalInEyeSpace);
    vec3 e = normalize(-vertexInEyeSpace);
    
    vec3 diffuseColor = kd;
    if (textured) {
        diffuseColor *= texture(diffuseTex, vec2(texCoord.x, 1 - texCoord.y)).xyz;
    }

    vec3 outColor = ke;
    
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
    
    finalColor = vec4(outColor, 1.0);
}
