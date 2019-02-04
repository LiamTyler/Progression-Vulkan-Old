#version 430 core

in vec3 TexCoords;

uniform samplerCube cubeMap;
uniform bool skybox;
uniform vec3 color;

out vec4 finalColor;

void main() {
    if (skybox) {
        finalColor = texture(cubeMap, TexCoords);
    } else {
        finalColor = vec4(color, 1);
    }
}
