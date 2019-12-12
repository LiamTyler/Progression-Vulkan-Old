#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/defines.h"
#include "graphics/shader_c_shared/structs.h"
#include "packing.h"

#define SCALE_RADIUS 0.5
#define BIAS 0.01

layout( location = 0 ) in vec2 UV;

layout( set = 0, binding = 0 ) uniform sampler2D worldPositions;
layout( set = 0, binding = 1 ) uniform sampler2D worldNormals;
layout( set = 0, binding = 2 ) uniform sampler2D ssaoNoise;

layout( set = 0, binding = 3 ) uniform SSAOKernel
{
	vec4 samples[PG_SSAO_KERNEL_SIZE];
} uboSSAOKernel;

layout( std430, push_constant ) uniform Matrices
{
    SSAOShaderData matrices;
};

layout( location = 0 ) out float occlusionFactor;


void main()
{
    vec3 fragPos = ( matrices.V * texture( worldPositions, UV ) ).xyz;
    vec3 N       = ( matrices.V * vec4( DecodeOctVec( texture( worldNormals, UV ).xyz ), 0 ) ).xyz;
    
    ivec2 texDim       = textureSize( worldPositions, 0 );
    ivec2 noiseDim     = textureSize( ssaoNoise, 0 );
    const vec2 noiseUV = vec2( float( texDim.x ) / float( noiseDim.x ), float( texDim.y ) / float( noiseDim.y ) ) * UV;  
	vec3 randomVec     = texture( ssaoNoise, noiseUV ).xyz;
    
    vec3 T   = normalize( randomVec - N * dot( N, randomVec ) );
    vec3 B   = cross( N, T );
    mat3 TBN = mat3( T, B, N );
    
    float occlusion = 0;
    for ( int i = 0; i < PG_SSAO_KERNEL_SIZE; ++i )
    {
        vec3 offsetPos = fragPos + TBN * uboSSAOKernel.samples[i].xyz * SCALE_RADIUS;
        
        vec4 projCoords = matrices.P * vec4( offsetPos, 1 );
        projCoords.xyz /= projCoords.w;
        projCoords.xy = 0.5 * projCoords.xy + vec2( 0.5 );
        projCoords.y = 1 - projCoords.y; // since current Vulkan viewport is inverted
        
        float offsetDepth = ( matrices.V * texture( worldPositions, projCoords.xy ) ).z;
        
        float rangeCheck = smoothstep(0.0f, 1.0f, SCALE_RADIUS / abs(fragPos.z - offsetDepth));
		occlusion += (offsetDepth >= offsetPos.z + BIAS ? 1.0f : 0.0f) * rangeCheck;
    }
    
    occlusionFactor = 1 - ( occlusion / float( PG_SSAO_KERNEL_SIZE ) );
}