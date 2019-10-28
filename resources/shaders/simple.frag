#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/standard_defines.h"

layout( location = 0 ) out vec4 outColor;

layout( location = 0 ) in vec3 posInWorldSpace;
layout( location = 1 ) in vec3 normalInWorldSpace;
layout( location = 2 ) in vec2 texCoord;

layout( set = 0, binding = 0 ) uniform PerSceneConstantBuffer
{
    mat4 VP;
    vec3 cameraPos;

} perSceneConstantBuffer;

layout( set = 2, binding = 0 ) uniform sampler2D textures[2048];

layout( std430, push_constant ) uniform MaterialConstantBuffer
{
    layout( offset = 128 ) vec4 Ka;
    layout( offset = 144 ) vec4 Kd;
    layout( offset = 160 ) vec4 Ks;
    layout( offset = 176 ) uint diffuseTexIndex;
} material;

void main()
{
    vec3 n = normalize( normalInWorldSpace );
    vec3 e = normalize( perSceneConstantBuffer.cameraPos - posInWorldSpace );
    
    vec3 lightDir   = normalize( vec3( 1, 0, -1 ) );
    vec3 lightColor = vec3( 1, 1, 1 );
    vec3 l = normalize( -lightDir );
    vec3 h = normalize( l + e );

    vec3 color = material.Ka.xyz;
    vec3 Kd    = material.Kd.xyz;
    if ( material.diffuseTexIndex != 65535 )
    {
        Kd *= texture( textures[material.diffuseTexIndex], texCoord ).xyz;
    }
    color += lightColor * Kd * max( 0.0, dot( l, n ) );
    if ( dot( l, n ) > PG_SHADER_EPSILON )
    {
        color += lightColor * material.Ks.xyz * pow( max( dot( h, n ), 0.0 ), 4*material.Ks.w );
    }
        
    outColor = vec4( color, 1.0 );
}
