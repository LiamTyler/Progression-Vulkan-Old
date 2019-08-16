#version 430 core

in vec3 vertex;
in vec3 normal;
in vec2 inTexCoord;

uniform mat4 M;
uniform mat4 N;
uniform mat4 MVP;
// uniform mat4 LSM;

out vec3 fragPosInWorldSpace;
out vec3 normalInWorldSpace;
out vec4 fragPosInLightSpace;
out vec2 texCoord;

void main() {
    fragPosInWorldSpace = (M * vec4(vertex, 1)).xyz;
    normalInWorldSpace  = (N * vec4(normal, 0)).xyz;
    // fragPosInLightSpace = LSM * vec4(fragPosInWorldSpace, 1);
    texCoord = inTexCoord;

    gl_Position = MVP * vec4(vertex, 1);
}
