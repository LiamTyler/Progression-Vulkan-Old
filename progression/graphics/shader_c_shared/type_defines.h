#ifdef PG_CPP_VERSION

#define VEC2 glm::vec2
#define VEC3 glm::vec3
#define VEC4 glm::vec4
#define MAT3 glm::mat3
#define MAT4 glm::mat4

#else // #ifdef PG_CPP_VERSION

#define VEC2 vec2
#define VEC3 vec3
#define VEC4 vec4
#define MAT3 mat3
#define MAT4 mat4

#endif // #else // #ifdef PG_CPP_VERSION
