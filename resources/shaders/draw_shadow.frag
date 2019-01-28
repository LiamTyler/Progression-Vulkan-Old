#version 430 core

in vec2 UV;

uniform sampler2D tex;

out vec4 finalColor;

void main() {
	finalColor = vec4(vec3(texture(tex, UV).r), 1);
}
