#version 430 core

in vec2 UV;

uniform sampler2D computeOutput;

out vec4 finalColor;

void main() {
    //finalColor = vec4(1, 0, 0, 1);
    finalColor = texture(computeOutput, UV);
}