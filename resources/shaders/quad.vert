#version 430 core

layout( location = 0 ) in vec3 vertex;

layout( location = 0 ) out vec2 UV;

void main() {
    gl_Position = vec4(vertex, 1);
    UV = .5 * (vertex.xy + vec2(1));
}
