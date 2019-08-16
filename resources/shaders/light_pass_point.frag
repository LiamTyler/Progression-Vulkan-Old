#version 430 core

#define EPSILON 0.000001

layout(location = 0) in vec4 glPos;

layout(binding = 1) uniform sampler2D gPosition;
layout(binding = 2) uniform sampler2D gNormal;
layout(binding = 3) uniform sampler2D gDiffuse;
layout(binding = 4) uniform sampler2D gSpecularExp;
layout(binding = 5) uniform sampler2D gEmissive;

// layout(binding = 6) uniform samplerCube shadowMap;
// layout(location = 7) uniform bool shadows;
// layout(location = 8) uniform float shadowFarPlane;

layout(location = 9) uniform vec3 cameraPos;

layout(location = 10) uniform vec3 lightPos;
layout(location = 11) uniform vec3 lightColor;
layout(location = 12) uniform float lightRadiusSquared;

layout(location = 0) out vec4 finalColor;

float attenuate(in const float distSquared, in const float radiusSquared) {
    float frac = distSquared / radiusSquared;
    float atten = max(0, 1 - frac * frac);
    return (atten * atten) / (1.0 + distSquared);
}

/*
float shadowAmount(in const vec3 fragPos, in const vec3 n) {
    vec3 fragToLight = fragPos - lightPos;
    // now get current linear depth as the length between the fragment and light position
    float currentDepth = length(fragToLight);
    if (currentDepth > shadowFarPlane)
        return 0.0;
    
    // use the light to fragment vector to sample from the depth map    
    float closestDepth = texture(shadowMap, fragToLight).r;
    // it is currently in linear range between [0,1]. Re-transform back to original value
    closestDepth *= shadowFarPlane;
    
    // now test for shadows
    float bias = 0.05; 
    // return 0;
    return currentDepth -  bias > closestDepth ? 1.0 : 0.0;
}
*/

void main() {    
    vec3 NDC = glPos.xyz / glPos.w;
    vec2 UV = .5 * (NDC.xy + vec2(1));

    vec3 fragPos      = texture(gPosition, UV).rgb;
    vec3 n            = texture(gNormal, UV).rgb;
    vec3 diffuseColor = texture(gDiffuse, UV).rgb;
    vec4 specExp      = texture(gSpecularExp, UV);
    vec3 ke           = texture(gEmissive, UV).rgb;

    vec3 e = normalize(cameraPos - fragPos);

    vec3 outColor = ke;
    
    vec3 vertToLight = lightPos - fragPos;
    vec3 l = normalize(vertToLight);
    vec3 h = normalize(l + e);

    float d2 = dot(vertToLight, vertToLight);
    float atten = attenuate(d2, lightRadiusSquared);

    outColor += atten * lightColor * diffuseColor * max(0.0, dot(l, n));
    if (dot(l, n) > EPSILON)
        outColor += atten * lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), 4*specExp.a);
    
    // if (shadows) {
    //     outColor *= (1.0f - shadowAmount(fragPos, n));
    // }

    finalColor.rgb = outColor;
    finalColor.a   = 1.0;
}
