#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 0 ) out vec3 posInWorldSpace;
layout( location = 1 ) out vec3 normalInWorldSpace;
layout( location = 2 ) out vec2 texCoord;

layout( set = 0, binding = 0 ) uniform PerSceneConstantBuffer
{
    mat4 VP;
} perSceneConstantBuffer;

layout( std430, push_constant ) uniform PerObjectData
{
    mat4 modelMatrix;
    mat4 normalMatrix;
} perObjectData;

void main()
{
    posInWorldSpace    = ( perObjectData.modelMatrix  * vec4( inPosition, 1 ) ).xyz;
    normalInWorldSpace = ( perObjectData.normalMatrix * vec4( inNormal,   0 ) ).xyz;
    texCoord           = inTexCoord;

    gl_Position         = perSceneConstantBuffer.VP * perObjectData.modelMatrix * vec4( inPosition, 1.0 );
}
