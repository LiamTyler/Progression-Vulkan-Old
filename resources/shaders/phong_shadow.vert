#version 430 core

in vec3 vertex;
in vec3 normal;
in vec2 inTexCoord;

uniform mat4 model;
uniform mat4 normalMatrix;
uniform mat4 VP;
uniform mat4 lightSpaceMatrix;

out vec3 fragPosInWorldSpace;
out vec3 normalInWorldSpace;
out vec4 fragPosInLightSpace;
out vec2 texCoord;

void main() {
    vec3 fragPos = (model * vec4(vertex, 1)).xyz;
    fragPosInWorldSpace = fragPos;
    normalInWorldSpace = normalize((normalMatrix * vec4(normal, 0)).xyz);
    texCoord = inTexCoord;

    fragPosInLightSpace = lightSpaceMatrix * vec4(fragPos, 1);
    gl_Position = VP * model * vec4(vertex, 1);
}