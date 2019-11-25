#version 450
#extension GL_ARB_separate_shader_objects : enable

layout( location = 0 ) in vec3 vertex;

layout( push_constant ) uniform transforms
{
	mat4 MVP;
};

void main()
{
    gl_Position = MVP * vec4( vertex, 1 );
}