#version 430 core

in vec3 vertex;

uniform mat4 MVP;

out vec3 TexCoords;

void main() {
    TexCoords = vertex;
    gl_Position = MVP * vec4(vertex, 1.0);
    gl_Position.z = gl_Position.w - 0.000001;
}