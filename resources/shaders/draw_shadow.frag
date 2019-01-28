#version 430 core

in vec2 UV;

uniform sampler2D tex;

out vec4 finalColor;

void main() {
    float depthValue = texture(tex, UV).r;
	finalColor = vec4(vec3(depthValue), 1);
}