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

float ShadowCalculation(in const vec4 fragPosInLS, in const vec3 n, in const vec3 lightDir) {
    vec3 ndc = fragPosInLS.xyz / fragPosInLS.w;
    vec3 projCoords = 0.5 * ndc + vec3(0.5);
    float currentDepth = projCoords.z;
    // To account for when the fragment is shadow's projection matrix doesn't reach far enough
    // so the depth would be > 1 and always be in shadow
    if (currentDepth > 1.0)
        return 1.0;


    float cosTheta = max(0.0, dot(n, lightDir));
    float bias = max(0.005 * (1.0 - cosTheta), 0.0005);
    // float bias = 0.0008*tan(acos(cosTheta));
    bias = clamp(bias, 0.0, 0.01);

    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(depthTex, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(depthTex, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // float shadowDepth = texture(depthTex, projCoords.xy).r;
    return currentDepth - bias > texture(depthTex, projCoords.xy).r ? 0.3 : 1.0;
    // return 1.0 - shadow;
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
    
    float shadow = ShadowCalculation(fragPosInLightSpace, n, l);
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
