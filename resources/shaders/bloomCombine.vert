#version 430 core

in vec2 vertex;

out vec2 UV;

void main() {
    gl_Position = vec4(vertex, 0, 1);
    UV = .5 * (vertex + vec2(1));
}