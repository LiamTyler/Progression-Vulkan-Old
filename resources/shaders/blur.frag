#version 430 core

in vec2 UV;

uniform sampler2D tex;
uniform vec2 offset;

out vec4 finalColor;

void main() {
    vec4 c = vec4(0);
    c += 5 * texture(tex, UV - offset);
    c += 6 * texture(tex, UV);
    c += 5 * texture(tex, UV + offset);
	
	finalColor = c / 16.0;
}