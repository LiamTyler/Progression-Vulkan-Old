#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 inPosition;
layout( location = 1 ) in vec3 inNormal;
layout( location = 2 ) in vec2 inTexCoord;

layout( location = 0 ) out vec3 posInWorldSpace;
layout( location = 1 ) out vec3 normalInWorldSpace;
layout( location = 2 ) out vec2 texCoord;

layout( set = 5, binding = 0 ) uniform PerObjectConstantBuffer
{
    mat4 modelMatrix;
    mat4 normalMatrix;
    mat4 MVP;
} perObjectConstantBuffer;

void main()
{
    posInWorldSpace    = ( perObjectConstantBuffer.modelMatrix  * vec4( inPosition, 1 ) ).xyz;
    normalInWorldSpace = ( perObjectConstantBuffer.normalMatrix * vec4( inNormal,   0 ) ).xyz;
    texCoord           = inTexCoord;

    gl_Position         = perObjectConstantBuffer.MVP * vec4( inPosition, 1.0 );
}
