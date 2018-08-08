#version 430 core

in vec3 TexCoords;

uniform samplerCube cubeMap;

out vec4 finalColor;

void main() {
    finalColor = texture(cubeMap, TexCoords);
}
