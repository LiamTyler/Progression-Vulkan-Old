#version 430 core

in vec2 UV;

uniform sampler2D tex;
uniform vec2 offset;

out vec4 finalColor;

void main() {
    vec4 c = vec4(0);
	
	c += 0.06136 * texture(tex, UV - 2*offset);
	c += 0.24477 * texture(tex, UV - offset);
	c += 0.38774 * texture(tex, UV);
	c += 0.24477 * texture(tex, UV + offset);
	c += 0.06136 * texture(tex, UV + 2*offset);
	
	finalColor = c;
	
	/*
    c += 5 * texture(tex, UV - offset);
    c += 6 * texture(tex, UV);
    c += 5 * texture(tex, UV + offset);
	finalColor = c / 16.0;
	*/
}