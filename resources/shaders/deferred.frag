#version 430 core

out vec4 fragColor;

in vec2 UV;

uniform sampler2D posTex;
uniform sampler2D normalTex;
uniform sampler2D colorTex;

uniform vec3 lightColor;
uniform vec3 lightInEyeSpace;

void main() {
    vec3 vertexInEyeSpace = texture(posTex, UV).xyz;
    vec3 normalInEyeSpace = texture(normalTex, UV).xyz;
    vec3 color = texture(colorTex, UV).xyz;
    
    vec3 n = normalize(normalInEyeSpace);
    vec3 l = normalize(-lightInEyeSpace);
    
    fragColor = vec4(color, 1);
    return;
    
    //vec3 e = normalize(-vertexInEyeSpace);
    //vec3 h = normalize(l + e);

    fragColor.rgb = lightColor * color * max(0.0, dot(l, n));
    fragColor.a   = 1.0;
}