#version 430 core

in vec3 vertex;

uniform mat4 VP;

out vec3 TexCoords;

void main() {
    TexCoords = -vertex;
    //TexCoords.y *= -1;
    TexCoords.x *= -1;
    gl_Position = VP * vec4(vertex, 1.0);
    gl_Position.z = gl_Position.w - 0.000001;
}
