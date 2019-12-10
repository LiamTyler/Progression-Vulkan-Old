#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/defines.h"
#include "graphics/shader_c_shared/structs.h"

layout( location = 0 ) out vec4 outPosition;
layout( location = 1 ) out vec4 outNormal;
layout( location = 2 ) out uvec4 outDiffuseAndSpecular;

layout( location = 0 ) in vec3 posInWorldSpace;
layout( location = 1 ) in vec2 texCoord;
layout( location = 2 ) in mat3 TBN;

layout( set = PG_2D_TEXTURES_SET, binding = 0 ) uniform sampler2D textures[PG_MAX_NUM_TEXTURES];

layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = PG_MATERIAL_PUSH_CONSTANT_OFFSET ) MaterialConstantBufferData material;
};

uint PackTwoFloatsToShort( in const float x, in const float y )
{
    uint uX = uint( x * 0xFF );
    uint uY = uint( y * 0xFF );
    return ( uX << 8 ) | ( uY & 0xFF );
}

uvec4 PackDiffuseAndSpecular( in const vec3 Kd, in const vec4 Ks )
{
    uvec4 enc;
    enc.x = PackTwoFloatsToShort( Kd.x, Ks.x );
    enc.y = PackTwoFloatsToShort( Kd.y, Ks.y );
    enc.z = PackTwoFloatsToShort( Kd.z, Ks.z );
    enc.w = uint( Ks.w );
    
    return enc;
}

void main()
{
    outPosition = vec4( posInWorldSpace, 1 );

    vec3 n = normalize( TBN[2] );
    if ( material.normalMapIndex != PG_INVALID_TEXTURE_INDEX )
    {
        n = texture( textures[material.normalMapIndex], texCoord ).xyz;
        n = normalize( n * 2 - 1 );
        n = normalize( TBN * n );
    }
    outNormal = vec4( n, 0 );
    
    vec3 Kd    = material.Kd.xyz;
    if ( material.diffuseTexIndex != PG_INVALID_TEXTURE_INDEX )
    {
        // Kd *= texture( textures[material.diffuseTexIndex], texCoord ).xyz;
        vec4 diff = texture( textures[material.diffuseTexIndex], texCoord );
        if ( diff.a < 0.01 )
        {
            discard;
        }
        Kd *= diff.xyz;
    }
    outDiffuseAndSpecular = PackDiffuseAndSpecular( Kd, material.Ks );
}
