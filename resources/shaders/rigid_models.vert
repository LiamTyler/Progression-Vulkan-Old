#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/structs.h"

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inTexCoord;
layout( location = 3 ) in vec3 inTangent;

layout( location = 0 ) out vec3 posInWorldSpace;
layout( location = 1 ) out mat3 TBN;
layout( location = 4 ) out vec2 texCoord;

layout( set = PG_SCENE_CONSTANT_BUFFER_SET, binding = 0 ) uniform SceneConstantBufferUniform
{
    SceneConstantBufferData sceneConstantBuffer;
};

layout( std430, push_constant ) uniform PerObjectData
{
    ObjectConstantBufferData perObjectData;
};

void main()
{
    posInWorldSpace = ( perObjectData.M * vec4( inPosition, 1 ) ).xyz;
    texCoord        = inTexCoord;
    
    vec3 T = normalize( ( perObjectData.M * vec4( inTangent, 0 ) ).xyz );
    vec3 N = normalize( ( perObjectData.N * vec4( inNormal,  0 ) ).xyz );
    vec3 B = normalize( cross( N, T ) );
    TBN    = mat3( T, B, N );
    
    gl_Position = sceneConstantBuffer.VP * perObjectData.M * vec4( inPosition, 1.0 );
}
