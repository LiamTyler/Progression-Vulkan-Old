#version 430 core

in vec3 vertex;
in vec3 normal;
in vec2 inTexCoord;

uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform mat4 projectionMatrix;

out vec3 vertexInEyeSpace;
out vec3 normalInEyeSpace;
out vec2 texCoord;

void main() {
    vertexInEyeSpace = (modelViewMatrix * vec4(vertex, 1)).xyz;
    normalInEyeSpace = normalize((normalMatrix * vec4(normal, 0)).xyz);
    texCoord = inTexCoord;

    gl_Position = projectionMatrix * modelViewMatrix * vec4(vertex, 1);
}
