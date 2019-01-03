#version 430 core

#define EPSILON 0.000001

in vec2 UV;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gDiffuse;
uniform sampler2D gSpecularExp;

uniform int numDirectionalLights;
uniform int numPointLights;

layout(std430, binding=10) buffer point_light_list
{
    vec4 lights[];
};

out vec4 finalColor;

void main() {    
    vec3 fragPos      = texture(gPosition, UV).rgb;
    vec3 n            = texture(gNormal, UV).rgb;
    vec3 diffuseColor = texture(gDiffuse, UV).rgb;
    vec4 specExp      = texture(gSpecularExp, UV);
    vec3 e = normalize(-fragPos);

    vec3 outColor = vec3(0, 0, 0);
    
    for (int i = 0; i < numDirectionalLights; ++i) {
        vec3 lightDir   = lights[2 * i + 0].rgb;
        vec3 lightColor = lights[2 * i + 1].rgb;
        vec3 l = normalize(-lightDir);
        vec3 h = normalize(l + e);
        outColor += lightColor * ka;
        outColor += lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            outColor += lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), specExp.a);        
    }
    
    for (int i = 0; i < numPointLights; ++i) {
        vec3 lightPos   = lights[2 * (numDirectionalLights + i) + 0].rgb;
        vec3 lightColor = lights[2 * (numDirectionalLights + i) + 1].rgb;

        vec3 l = normalize(lightPos - fragPos);
        vec3 h = normalize(l + e);
        float attenuation = 1.0 / pow(length(lightPos - fragPos), 2.0);
        // outColor += lightColor * ka;
        outColor += attenuation * lightColor * diffuseColor * max(0.0, dot(l, n));
        if (dot(l, n) > EPSILON)
            outColor += attenuation * lightColor * specExp.rgb * pow(max(dot(h, n), 0.0), specExp.a);        
    }
    
    
    finalColor.rgb = outColor;
    finalColor.a   = 1.0;
}