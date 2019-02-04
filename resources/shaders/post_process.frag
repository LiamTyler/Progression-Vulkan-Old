#version 430 core

in vec2 UV;

uniform sampler2D originalColor;
uniform float exposure;

out vec4 finalColor;

const float gamma = 2.2;

void main() {
    vec3 hdrColor = texture(originalColor, UV).rgb;
	
    
    /*
	// Exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    // Gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
	
	finalColor = vec4(mapped, 1.0);
    */
    finalColor.rgb = pow(hdrColor, vec3(1.0/gamma));
    // finalColor.rgb = hdrColor;
    finalColor.a = 1.0;
}