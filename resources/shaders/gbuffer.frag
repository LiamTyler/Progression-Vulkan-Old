#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/defines.h"
#include "graphics/shader_c_shared/structs.h"
#include "packing.h"

layout( location = 0 ) out vec4 outNormal;
layout( location = 1 ) out vec4 outDiffuse;
layout( location = 2 ) out vec2 outMetallicAndRoughness;

layout( location = 0 ) in vec3 posInWorldSpace;
layout( location = 1 ) in vec2 texCoord;
layout( location = 2 ) in mat3 TBN;

layout( set = PG_2D_TEXTURES_SET, binding = 0 ) uniform sampler2D textures[PG_MAX_NUM_TEXTURES];

layout( std430, push_constant ) uniform MaterialConstantBufferUniform
{
    layout( offset = PG_MATERIAL_PUSH_CONSTANT_OFFSET ) MaterialConstantBufferData material;
};

void main()
{
    vec3 n = normalize( TBN[2] );
    if ( material.normalTexIndex != PG_INVALID_TEXTURE_INDEX )
    {
        n.xy = texture( textures[material.normalTexIndex], texCoord ).xy;
        n.xy = 2 * n.xy - 1;
        n.z = sqrt( 1 - n.x * n.x + n.y * n.y );
        n = normalize( TBN * n );
    }
    
    vec3 Kd    = material.Kd.xyz;
    if ( material.diffuseTexIndex != PG_INVALID_TEXTURE_INDEX )
    {
        vec4 diff = texture( textures[material.diffuseTexIndex], texCoord );
        if ( diff.a < 0.01 )
        {
            discard;
        }
        Kd *= diff.xyz;
    }
    
    float metallic = material.metallic;
    if ( material.metallicTexIndex != PG_INVALID_TEXTURE_INDEX )
    {
        metallic *= texture( textures[material.metallicTexIndex], texCoord ).r;
    }
    float roughness = material.roughness;
    if ( material.metallicTexIndex != PG_INVALID_TEXTURE_INDEX )
    {
        roughness *= texture( textures[material.roughnessTexIndex], texCoord ).r;
    }
    
    
    //outPosition             = vec4( posInWorldSpace, 1 );
    outNormal               = vec4( EncodeOctVec( n ), 0 );
    outDiffuse              = vec4( Kd, 0 );
    outMetallicAndRoughness = vec2( metallic, roughness );
}
