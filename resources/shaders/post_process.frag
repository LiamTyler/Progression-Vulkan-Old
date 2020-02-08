#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/structs.h"

layout( location = 0 ) in vec2 UV;
layout( location = 0 ) out vec4 finalColor;

layout( set = 0, binding = 0 ) uniform sampler2D originalColor;
layout( std430, push_constant ) uniform CameraData
{
    PostProcessConstantBufferData camera;
#if PG_DEBUG_BUILD
    DebugRenderData debugData;
#endif // #if PG_DEBUG_BUILD
};

// http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 Uncharted2Tonemap( vec3 x )
{
	float A = 0.15;
	float B = 0.50;
	float C = 0.10;
	float D = 0.20;
	float E = 0.02;
	float F = 0.30;
	return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 ReinhardTonemap( vec3 x )
{
    return x / (x + vec3( 1.0 ));
}

vec3 Tonemap( vec3 x )
{
    // x = Uncharted2Tonemap( x * camera.exposure );
    // vec3 whiteScale = 1.0f / Uncharted2Tonemap( vec3( 11.2f ) );
    // x *= whiteScale;
    
    x = ReinhardTonemap( x );
    
    return x;
}

void main()
{
    vec3 hdrColor = texture( originalColor, UV ).rgb;
    vec3 color    = hdrColor;
    
#if PG_DEBUG_BUILD
    if ( debugData.tonemap )
    {
        color = Tonemap( hdrColor );
    }
    if ( debugData.gammaCorrect )
    {
        color = pow( color, vec3( 1.0f / camera.gamma ) );
    }
#else // #if PG_DEBUG_BUILD

    // Tone mapping
	color = Tonemap( hdrColor );
	// Gamma correction
	color = pow( color, vec3( 1.0f / camera.gamma ) );
    
#endif // #else // #if PG_DEBUG_BUILD
    
    finalColor.rgb = color;
    finalColor.a = 1;
}