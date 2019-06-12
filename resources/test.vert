#version 430 core

in vec2 vertex;
in vec3 normal;
in vec2 uv;

out vec3 fragNormal;
out vec2 UV;

void main() {
    gl_Position = vec4(vertex, 0, 1);
    fragNormal = normal;
    // UV = .5 * (vertex + vec2(1));
}
