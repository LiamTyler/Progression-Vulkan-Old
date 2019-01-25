#version 430 core

in vec3 vertex;

uniform mat4 VP;

out vec3 TexCoords;

void main() {
    TexCoords = vertex;
    gl_Position = VP * vec4(.1f * vertex, 1.0);
    gl_Position.z = gl_Position.w - 0.000001;
}
