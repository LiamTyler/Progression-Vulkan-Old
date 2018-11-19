#version 430 core

in vec2 UV;

uniform sampler2D originalColor;
uniform sampler2D blur1;
uniform sampler2D blur2;
uniform sampler2D blur3;
uniform sampler2D blur4;
//uniform sampler2D blur5;

uniform bool bloom;
uniform float bloomIntensity;
uniform float exposure;

out vec4 finalColor;

void main() {
    vec4 original = texture(originalColor, UV);
    vec4 bloomColor  = vec4(0);
    if (bloom) {
        vec4 b1       = texture(blur1, UV);
        vec4 b2       = texture(blur2, UV);
        vec4 b3       = texture(blur3, UV);
        vec4 b4       = texture(blur4, UV);
        //vec4 b5       = texture(blur4, UV);
        
        bloomColor = b1 + b2 + b3 + b4;
    }
	vec3 hdrColor = original.rgb + bloomIntensity * bloomColor.rgb;
	
	const float gamma = 2.2;
	// Exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    // Gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
	
	finalColor = vec4(mapped, 1.0);
	finalColor.rgb = hdrColor;
}