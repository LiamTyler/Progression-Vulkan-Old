#pragma once

#define PG_SHADER_EPSILON 0.00001

#define PG_MAX_NUM_TEXTURES 2048
#define PG_INVALID_TEXTURE_INDEX 65535

#define PG_SCENE_CONSTANT_BUFFER_SET 0
#define PG_2D_TEXTURES_SET 1
#define PG_BONE_TRANSFORMS_SET 2

#define PG_MATERIAL_PUSH_CONSTANT_OFFSET 192

#ifdef PG_CPP_VERSION

#define PG_NAMESPACE_BEGIN namespace Progression {
#define PG_NAMESPACE_END } // namespace Progression

#define VEC2 glm::vec2
#define VEC3 glm::vec3
#define VEC4 glm::vec4
#define UVEC2 glm::uvec2
#define UVEC3 glm::uvec3
#define UVEC4 glm::uvec4
#define IVEC2 glm::ivec2
#define IVEC3 glm::ivec3
#define IVEC4 glm::ivec4
#define MAT3 glm::mat3
#define MAT4 glm::mat4
#define UINT uint32_t

#else // #ifdef PG_CPP_VERSION

#define PG_NAMESPACE_BEGIN
#define PG_NAMESPACE_END

#define VEC2 vec2
#define VEC3 vec3
#define VEC4 vec4
#define UVEC2 uvec2
#define UVEC3 uvec3
#define UVEC4 uvec4
#define IVEC2 ivec2
#define IVEC3 ivec3
#define IVEC4 ivec4
#define MAT3 mat3
#define MAT4 mat4
#define UINT uint

#endif // #else // #ifdef PG_CPP_VERSION