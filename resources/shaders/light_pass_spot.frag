#version 430 core

#define EPSILON 0.000001

in vec4 glPos;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecularExp;
uniform sampler2D gEmissive;

uniform vec3 cameraPos;

uniform sampler2D shadowMap;
uniform bool shadows;
uniform mat4 LSM;

uniform vec3 lightPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float lightInnerCutoff;
uniform float lightOuterCutoff;
uniform float lightRadiusSquared;

out vec4 finalColor;


float attenuate(in const float distSquared, in const float radiusSquared) {
    float frac = distSquared / radiusSquared;
    float atten = max(0, 1 - frac * frac);
    return (atten * atten) / (1.0 + distSquared);
}

// return how much the fragment is 'in' the shadow.
// 0 == not at all
// 1 == fully shadowed
float shadowAmount(const in vec3 fragPos, const in vec3 n, const in vec3 l) {
    vec4 fragPosInLightSpace = LSM * vec4(fragPos, 1);
    vec3 ndc = fragPosInLightSpace.xyz / fragPosInLightSpace.w;
    vec3 projCoords = 0.5 * ndc + vec3(0.5);
    float currentDepth = projCoords.z;
    // To account for when the fragment is shadow's projection matrix doesn't reach far enough
    // so the depth would be > 1 and always be in shadow
    if (currentDepth > 1.0)
        return 0;

    float cosTheta = max(0.0, dot(n, l));
    float bias = max(0.002 * (1.0 - cosTheta), 0.0005);
    // float bias = 0.0008*tan(acos(cosTheta));
    bias = clamp(bias, 0.0, 0.01);

    return currentDepth - bias > texture(shadowMap, projCoords.xy).r ? 1.0 : 0.0;
}

void main() {    
    vec3 NDC = glPos.xyz / glPos.w;
    vec2 UV = .5 * (NDC.xy + vec2(1));

    vec3 fragPos      = texture(gPosition, UV).rgb;
    vec3 n            = texture(gNormal, UV).rgb;
    vec3 diffuseColor = texture(gDiffuse, UV).rgb;
    vec4 specExp      = texture(gSpecularExp, UV);
    vec3 ke           = texture(gEmissive, UV).rgb;


    vec3 outColor = ke;
    
    vec3 vertToLight = lightPos - fragPos;
    vec3 l = normalize(vertToLight);

    float theta = dot(-l, lightDir);
    if (theta > lightOuterCutoff) {
        vec3 e = normalize(cameraPos - fragPos);
        vec3 h = normalize(l + e);
        float epsilon = lightInnerCutoff - lightOuterCutoff;
        float intensity = clamp((theta - lightOuterCutoff) / epsilon, 0.0, 1.0);
        float d2 = dot(vertToLight, vertToLight);
        float atten = intensity * attenuate(d2, lightRadiusSquared);

        outColor += atten * lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            outColor += atten * lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), 4*specExp.a);

        if (shadows) {
            outColor *= (1.0f - shadowAmount(fragPos, n, l));
        }
    }

    finalColor.rgb = outColor;
    finalColor.a   = 1.0;
}
