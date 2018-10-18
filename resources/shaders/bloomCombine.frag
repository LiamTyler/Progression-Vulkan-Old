#version 430 core

in vec2 UV;

uniform sampler2D originalColor;
uniform sampler2D glow0;

out vec4 finalColor;

void main() {
    vec4 original = texture(originalColor, UV);
	vec4 g1       = texture(glow0, UV);
	
	finalColor = clamp(original + g1, 0.0, 1.0);
	//finalColor = g1;
}