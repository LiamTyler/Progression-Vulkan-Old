#version 430 core

in vec3 vertex;

uniform mat4 MVP;

out vec4 glPos;

void main() {
    gl_Position = MVP * vec4(vertex, 1);
    glPos = gl_Position;
}
