#version 430 core

in vec2 UV;

uniform sampler2D originalColor;
uniform sampler2D blur1;
uniform sampler2D blur2;
uniform sampler2D blur3;
uniform sampler2D blur4;

uniform float bloomIntensity;

out vec4 finalColor;

void main() {
    vec4 original = texture(originalColor, UV);
	vec4 b1       = texture(blur1, UV);
	vec4 b2       = texture(blur2, UV);
	vec4 b3       = texture(blur3, UV);
	vec4 b4       = texture(blur4, UV);
	
	vec4 bloom = b1 + b2 + b3 + b4;
	finalColor = clamp(original + bloomIntensity * bloom, 0.0, 1.0);
	//finalColor = b4;
}