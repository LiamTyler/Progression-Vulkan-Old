#version 430 core

in vec3 fragNormal;
in vec2 UV;

// uniform sampler2D tex;

out vec4 finalColor;

void main() {
	// finalColor = texture(tex, UV);
    finalColor = vec4(1, 1, 0, 1);
}
