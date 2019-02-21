#version 430 core

#define EPSILON 0.000001

in vec2 UV;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecularExp;
uniform sampler2D gEmissive;

uniform vec3 cameraPos;

uniform vec3 lightDir;
uniform vec3 lightColor;

out vec4 finalColor;

void main() {    
    vec3 fragPos      = texture(gPosition, UV).rgb;
    vec3 n            = texture(gNormal, UV).rgb;
    vec3 diffuseColor = texture(gDiffuse, UV).rgb;
    vec4 specExp      = texture(gSpecularExp, UV);
    vec3 ke           = texture(gEmissive, UV).rgb;

    vec3 e = normalize(cameraPos - fragPos);

    vec3 outColor = ke;
    
    vec3 l = normalize(-lightDir);
    vec3 h = normalize(l + e);
    // outColor += lightColor * ka;
    outColor += lightColor * diffuseColor * max(0.0, dot(l, n));
    if (dot(l, n) > EPSILON)
        outColor += lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), specExp.a);        
    
    finalColor.rgb = outColor;
    finalColor.a   = 1.0;
}
