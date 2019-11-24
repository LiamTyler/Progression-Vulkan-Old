#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec2 UV;

layout( set = 0,  binding = 0) uniform sampler2D originalColor;
// uniform float exposure;

layout(location = 0 ) out vec4 finalColor;

const float gamma = 2.2;

void main() {
    vec3 hdrColor = texture( originalColor, UV ).rgb;
	
	// // Exposure tone mapping // TODO (exposure always 1 right now)
    // vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    // // Gamma correction 
    // mapped = pow(mapped, vec3(1.0 / gamma));
	// finalColor = vec4(mapped, 1.0);

    // Gamma correction 
    finalColor.rgb = pow(hdrColor, vec3(1.0 / gamma));
    finalColor.a = 1;
	//finalColor.rgba = vec4(1.0f,0,0,1);
}