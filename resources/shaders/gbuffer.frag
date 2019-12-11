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

vec2 signNotZero( vec2 v )
{
    return vec2( ( v.x >= 0.0 ) ? 1.0 : -1.0, ( v.y >= 0.0 ) ? 1.0 : -1.0 );
}

// Assume normalized input. Output is on [-1, 1] for each component.
vec2 float32x3_to_oct( in vec3 v )
{
    // Project the sphere onto the octahedron, and then onto the xy plane
    vec2 p = v.xy * ( 1.0 / ( abs( v.x ) + abs( v.y ) + abs( v.z ) ) );
    
    // Reflect the folds of the lower hemisphere over the diagonals
    return ( v.z >= 0.0 ) ? p : ( ( 1.0 - abs( p.yx ) ) * signNotZero( p ) );
}

vec3 snorm12x2_to_unorm8x3( vec2 f )
{
    // renormalize from [-1, 1] --> [0, 4094] = 12 bits
    vec2 u = vec2( round( clamp( f, -1.0, 1.0 ) * 2047 + 2047 ) );
    float t = floor( u.y / 256.0 ); // upper 4 bits of f.y
    // If storing to GL_RGB8UI, omit the final division
    return floor( vec3(
        u.x / 16.0, // most significant 8 bits of f.x 
        fract( u.x / 16.0 ) * 256.0 + t, // upper 4 bits = least significant 4 bits of u.x, lower 4 bits = most significant 4 bits of u.y
        u.y - t * 256.0 ) // least significant 8 bits of u.y
    ) / 255.0;
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
    //outNormal = vec4( n, 0 );
    outNormal = vec4( snorm12x2_to_unorm8x3 ( float32x3_to_oct( n ) ), 0 );
    
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
