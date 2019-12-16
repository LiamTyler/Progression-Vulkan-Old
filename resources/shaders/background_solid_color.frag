#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec2 UV;

layout( location = 0 ) out vec4 finalColor;

layout( std430, push_constant ) uniform PushConstantColor
{
    layout( offset = 0 ) vec4 color;
};

void main()
{
    finalColor = color;
}