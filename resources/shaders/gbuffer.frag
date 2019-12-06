#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/defines.h"
#include "graphics/shader_c_shared/structs.h"

layout( location = 0 ) out vec4 outPosition;
layout( location = 1 ) out vec4 outNormal;
layout( location = 2 ) out vec4 outDiffuse;
layout( location = 3 ) out vec4 outSpecular;

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
        Kd *= texture( textures[material.diffuseTexIndex], texCoord ).xyz;
    }
    outDiffuse = vec4( Kd, 0 );
    
    outSpecular = material.Ks;
}
