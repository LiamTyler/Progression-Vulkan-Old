#version 450
#extension GL_ARB_separate_shader_objects : enable

#include "graphics/shader_c_shared/structs.h"

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 0 ) out vec3 posInWorldSpace;
layout( location = 1 ) out vec3 normalInWorldSpace;
layout( location = 2 ) out vec2 texCoord;

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
    posInWorldSpace    = ( perObjectData.M  * vec4( inPosition, 1 ) ).xyz;
    normalInWorldSpace = ( perObjectData.N  * vec4( inNormal,   0 ) ).xyz;
    texCoord           = inTexCoord;
    gl_Position        = sceneConstantBuffer.VP * perObjectData.M * vec4( inPosition, 1.0 );
}