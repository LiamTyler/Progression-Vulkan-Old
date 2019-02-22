#version 430 core

layout(location = 0) in vec3 vertex;

layout(location = 0) uniform mat4 MVP;

layout(location = 0) out vec4 glPos;

void main() {
    gl_Position = MVP * vec4(vertex, 1);
    glPos = gl_Position;
}
