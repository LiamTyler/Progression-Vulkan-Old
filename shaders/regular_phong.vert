#version 330 core

in vec3 vertex;
in vec3 normal;

uniform mat4 modelViewMatrix;
uniform mat4 normalMatrix;
uniform mat4 projectionMatrix;

out vec3 vertexInEyeSpace;
out vec3 normalInEyeSpace;

void main() {
    vertexInEyeSpace = (modelViewMatrix * vec4(vertex, 1)).xyz;
    normalInEyeSpace = normalize((normalMatrix * vec4(normal, 0)).xyz);

    gl_Position = projectionMatrix * modelViewMatrix * vec4(vertex, 1);
}
