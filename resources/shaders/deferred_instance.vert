#version 430 core

in vec3 vertex;
in vec3 normal;
in vec2 inTexCoord;
in mat4 MV;

uniform mat4 V;
uniform mat4 projectionMatrix;

out vec3 vertexInEyeSpace;
out vec3 normalInEyeSpace;
out vec2 texCoord;

void main() {
    mat4 normalMatrix = transpose(inverse(MV));
    vertexInEyeSpace = (MV * vec4(vertex, 1)).xyz;
    normalInEyeSpace = normalize((normalMatrix * vec4(normal, 0)).xyz);
    texCoord = inTexCoord;

    gl_Position = projectionMatrix * MV * vec4(vertex, 1);
}
