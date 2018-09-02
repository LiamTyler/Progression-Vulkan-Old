#version 430 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 vertexInEyeSpace;
in vec3 normalInEyeSpace;
in vec2 texCoord;

uniform vec3 kd;
uniform float specular;

uniform bool textured;
uniform sampler2D diffuseTex;

void main() {
    gPosition = vertexInEyeSpace;
    
    gNormal = normalize(normalInEyeSpace);
    
    gAlbedoSpec = vec4(kd, 1);
    gAlbedoSpec = vec4(1, 0, 0, 1);
    
    
    //if (textured) {
    //    gAlbedoSpec.xyz = texture(diffuseTex, vec2(texCoord.x, 1 - texCoord.y)).xyz;
    //}
    //gAlbedoSpec.a = specular;
}
/*
in vec3 posWorldSpace;
in vec3 normalWorldSpace;
in vec2 texCoord;

uniform vec3 kd;
uniform float specular;

uniform bool textured;
uniform sampler2D diffuseTex;

void main() {
    gPosition = posWorldSpace;
    
    gNormal = normalWorldSpace;
    
    gAlbedoSpec.xyz = kd;
    if (textured) {
        gAlbedoSpec.xyz = texture(diffuseTex, vec2(texCoord.x, 1 - texCoord.y)).xyz;
    }
    gAlbedoSpec.a = specular;
}
*/