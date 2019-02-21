#version 430 core

#define EPSILON 0.000001

in vec4 glPos;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecularExp;
uniform sampler2D gEmissive;

uniform vec3 cameraPos;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float lightRadiusSquared;

out vec4 finalColor;


float attenuate(in const float distSquared, in const float radiusSquared) {
    float frac = distSquared / radiusSquared;
    float atten = max(0, 1 - frac * frac);
    return (atten * atten) / (1.0 + distSquared);
}

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
    
    finalColor.rgb = outColor;
    finalColor.a   = 1.0;
}
