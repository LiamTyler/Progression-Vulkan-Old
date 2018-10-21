#version 430 core

in vec2 UV;

uniform sampler2D originalColor;
uniform sampler2D fullGlow;
uniform sampler2D halfGlow;
uniform sampler2D quarterGlow;
uniform sampler2D eigthGlow;

out vec4 finalColor;

void main() {
    vec4 original = texture(originalColor, UV);
	vec4 g1       = texture(fullGlow, UV);
	vec4 g2       = texture(halfGlow, UV);
	vec4 g3       = texture(quarterGlow, UV);
	vec4 g4       = texture(eigthGlow, UV);
	
	finalColor = clamp(original + g1 + g2 + g3 + g4, 0.0, 1.0);
	// finalColor = clamp(original + g1 + g4, 0.0, 1.0);
	// finalColor = original;
}