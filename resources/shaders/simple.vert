#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 inPosition;
// layout( location = 1 ) in vec3 inNormal;
layout( location = 1 ) in vec2 inTexCoord;

layout( location = 0 ) out vec2 fragTexCoord;
// layout( location = 0 ) out vec3 fragPosInWorldSpace;
// layout( location = 1 ) out vec3 normalInWorldSpace;

layout( binding = 0 ) uniform UniformBufferObject {
    mat4 MVP;
} ubo;

void main()
{
    // fragPosInWorldSpace = ( ubo.M * vec4( inPosition, 1 ) ).xyz;
    //normalInWorldSpace  = ( ubo.N * vec4( inNormal, 0 ) ).xyz;
    gl_Position         = ubo.MVP * vec4( inPosition, 1.0 );
    fragTexCoord = inTexCoord;
}
