#version 430 core

in vec2 UV;

uniform sampler2D tex;
uniform vec2 offset;

uniform float kernel[100];
uniform int halfKernelWidth;

out vec4 finalColor;

void main() {
    vec4 c = vec4(0);
	
	for (int i = 0; i < 2*halfKernelWidth + 1; ++i) {
		c += kernel[i] * texture(tex, UV + (i - halfKernelWidth) * offset);
	}
	
	/*
	c += 0.06136 * texture(tex, UV - 2*offset);
	c += 0.24477 * texture(tex, UV - offset);
	c += 0.38774 * texture(tex, UV);
	c += 0.24477 * texture(tex, UV + offset);
	c += 0.06136 * texture(tex, UV + 2*offset);
	*/
	/*
    c += 5 * texture(tex, UV - offset);
    c += 6 * texture(tex, UV);
    c += 5 * texture(tex, UV + offset);
	c /= 16.0;
	*/
	finalColor = c;
}