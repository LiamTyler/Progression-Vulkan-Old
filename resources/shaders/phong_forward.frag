#version 430 core

#define EPSILON 0.000001

struct Light {
    vec3 pos;
    float rSquared;
    vec3 color;
    float innerCutoff;
    vec3 dir;
    float outterCutoff;
};

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

// shadow data
uniform int shadowLightType;
uniform Light shadowLight;
uniform sampler2D depthTex;

uniform vec3 ambientLight;
uniform int numDirectionalLights;
uniform int numPointLights;
uniform int numSpotLights;

layout(std430, binding=10) buffer point_light_list
{
    vec4 lights[];
};

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

    /*
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
    return 1.0 - shadow;
    */

    return currentDepth - bias > texture(depthTex, projCoords.xy).r ? 0.3 : 1.0;
}

float attenuate(in const float distSquared, in const float radiusSquared) {
    float frac = distSquared / radiusSquared;
    float atten = max(0, 1 - frac * frac);
    return (atten * atten) / (1.0 + distSquared);
}

void main() {
    vec3 n = normalize(normalInWorldSpace);
    vec3 e = normalize(cameraPos - fragPosInWorldSpace);
    
    vec3 diffuseColor = kd;
    if (textured) {
        diffuseColor *= texture(diffuseTex, texCoord).xyz;
    }

    vec3 outColor = ke + ka*ambientLight;

    vec3 l = shadowLight.dir;
    vec3 h = normalize(l + e);
    vec3 shadowColor = shadowLight.color * diffuseColor * max(0.0, dot(l, n));
    if (dot(l, n) > EPSILON)
        shadowColor += shadowLight.color * ks * pow(max(dot(h, n), 0.0), 4*specular);
    
    float shadow = ShadowCalculation(fragPosInLightSpace, n, l);
    outColor += shadow * shadowColor;
    
    /*
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

        vec3 vertToLight = lightPos - fragPosInWorldSpace;
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

        vec3 vertToLight = lightPos - fragPosInWorldSpace;
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
    */
    
    finalColor = vec4(outColor, 1.0);
}
