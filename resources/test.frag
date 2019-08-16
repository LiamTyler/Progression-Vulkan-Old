#version 430 core

#define EPSILON 0.000001

uniform vec3 cameraPos;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform vec3 ke;
uniform float shininess;

uniform sampler2D diffuseTex;
uniform bool textured;

uniform vec3 lightDirInWorldSpace;

in vec3 fragPosInWorldSpace;
in vec3 normalInWorldSpace;
in vec4 fragPosInLightSpace;
in vec2 texCoord;

out vec4 finalColor;

void main() {
	//finalColor = vec4(1, 0, 0, 1);
	//return;
    vec3 e = normalize(cameraPos - fragPosInWorldSpace);
    vec3 n = normalize(normalInWorldSpace);
    vec3 l = normalize(lightDirInWorldSpace);
    vec3 h = normalize(l + e);

    vec3 outColor = ke;
    
    vec3 diffuseColor = kd;
    if (textured)
        diffuseColor *= texture(diffuseTex, texCoord).rgb;
    outColor += diffuseColor * max(0.0, dot(n, vec3(0, 0, 1)));

    if (dot(l, n) > EPSILON)
        outColor += ks * pow(max(dot(h, n), 0.0), 4*shininess);        

    
    finalColor.rgb = outColor;
    // finalColor.g += 1;
    finalColor.a   = 1.0;
}
