#version 430 core

in vec2 UV;

uniform sampler2D originalColor;
uniform sampler2D uTexture1;

out vec4 finalColor;

void main() {
    vec4 original = texture(originalColor, UV);
	vec4 uT1      = texture(uTexture1, UV);
	
	finalColor = clamp(original + uT1, 0.0, 1.0);
}