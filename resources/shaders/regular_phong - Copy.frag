#version 430 core

in vec3 vertexInEyeSpace;
in vec3 normalInEyeSpace;
in vec2 texCoord;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float specular;

uniform bool textured;
uniform sampler2D diffuseTex;

uniform vec3 lightColor;

uniform vec3 lightInEyeSpace;

out vec4 finalColor;

void main() {
    vec3 n = normalize(normalInEyeSpace);
    vec3 l = normalize(-lightInEyeSpace);
    vec3 e = normalize(-vertexInEyeSpace);
    vec3 h = normalize(l + e);

    vec3 diffuseColor = vec3(1, 1, 1);
    if (textured) {
        diffuseColor = texture(diffuseTex, vec2(texCoord.x, 1 - texCoord.y)).xyz;
    }

    vec3 outColor = vec3(0, 0, 0);
    // outColor += lightColor * ka;
    outColor += lightColor * kd * diffuseColor * max(0.0, dot(l, n));
    outColor += lightColor * ks * pow(max(dot(h, n), 0.0), specular);

    finalColor.rgb = outColor;
    finalColor.a   = 1.0;
}
