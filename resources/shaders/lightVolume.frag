#version 430 core

in vec4 glPos;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecularExp;

uniform vec3 lightPos;
uniform vec3 lightColor;

out vec4 finalColor;

void main() {
    vec3 NDC = glPos.xyz / glPos.w;
    vec2 UV = .5 * (NDC.xy + vec2(1));
    vec3 fragPos      = texture(gPosition, UV).rgb;
    vec3 n            = texture(gNormal, UV).rgb;
    vec3 diffuseColor = texture(gDiffuse, UV).rgb;
    vec4 specExp      = texture(gSpecularExp, UV);
    vec3 e = normalize(-fragPos);

    vec3 outColor = vec3(0, 0, 0);

    vec3 l = normalize(lightPos - fragPos);
    vec3 h = normalize(l + e);
    float attenuation = 1.0 / pow(length(lightPos - fragPos), 2.0);
    
    // Blinn-phong
    // outColor += lightColor * ka;
    outColor += attenuation * lightColor * diffuseColor * max(0.0, dot(l, n));
    outColor += attenuation * lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), specExp.a);
    
    
    finalColor.rgb = outColor;
    finalColor.a   = 1.0;
}