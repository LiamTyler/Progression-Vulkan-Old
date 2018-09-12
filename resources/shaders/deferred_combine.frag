#version 430 core

in vec2 UV;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecularExp;

uniform vec3 lights[800];
uniform int numDirectionalLights;
uniform int numPointLights;

out vec4 finalColor;

void main() {    
    vec3 fragPos      = texture(gPosition, UV).rgb;
    vec3 n            = texture(gNormal, UV).rgb;
    vec3 diffuseColor = texture(gDiffuse, UV).rgb;
    vec4 specExp      = texture(gSpecularExp, UV);
    // specExp.a = 96.078431;
    vec3 e = normalize(-fragPos);

    vec3 outColor = vec3(0, 0, 0);
    
    for (int i = 0; i < numDirectionalLights; ++i) {
        vec3 lightDir   = lights[2 * i + 0];
        vec3 lightColor = lights[2 * i + 1];
        vec3 l = normalize(-lightDir);
        vec3 h = normalize(l + e);
        // outColor += lightColor * ka;
        outColor += lightColor * diffuseColor * max(0.0, dot(l, n));
        outColor += lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), specExp.a);        
    }
    
    for (int i = 0; i < numPointLights; ++i) {
        vec3 lightPos   = lights[2 * (numDirectionalLights + i) + 0];
        vec3 lightColor = lights[2 * (numDirectionalLights + i) + 1];

        vec3 l = normalize(lightPos - fragPos);
        vec3 h = normalize(l + e);
        float attenuation = 1.0 / pow(length(lightPos - fragPos), 2.0);
        // outColor += lightColor * ka;
        outColor += attenuation * lightColor * diffuseColor * max(0.0, dot(l, n));
        outColor += attenuation * lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), specExp.a);        
    }
    
    
    finalColor.rgb = outColor;
    finalColor.a   = 1.0;
}