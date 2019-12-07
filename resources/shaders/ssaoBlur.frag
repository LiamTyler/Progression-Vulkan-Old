#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec2 UV;

layout( set = 0, binding = 0 ) uniform sampler2D ssaoTex;

layout( location = 0 ) out float outFragColor;

#define BLUR_KERNEL_SIZE 2
#define BLUR_KERNEL_TOTAL_SIZE ( 2 * BLUR_KERNEL_SIZE + 1 )

void main()
{
    vec2 texDims = vec2( 1.0 ) / textureSize( ssaoTex, 0 );
    float total = 0;
    for ( int c = -BLUR_KERNEL_SIZE; c <= BLUR_KERNEL_SIZE; ++c )
    {
        for ( int r = -BLUR_KERNEL_SIZE; r <= BLUR_KERNEL_SIZE; ++r )
        {
            vec2 newUV = UV + vec2( c, r ) * texDims;
            total += texture( ssaoTex, newUV ).r;
        }
    }
    
    outFragColor = total / ( BLUR_KERNEL_TOTAL_SIZE * BLUR_KERNEL_TOTAL_SIZE );
}