#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "core/platform_defines.hpp"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include <iostream>
#include <memory>
#include <string>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PG_UNUSED( x ) (void) ( x );

#define PG_DEBUG_MODE

#ifdef PG_DEBUG_MODE
#define PG_ASSERT( x )                                                                       \
    if ( !( x ) )                                                                            \
    {                                                                                        \
        printf( "Failed assertion: (%s) at line %d in file %s.\n", #x, __LINE__, __FILE__ ); \
        abort();                                                                             \
    }
#else
#define PG_ASSERT( x )
#endif

inline std::ostream& operator<<( std::ostream& out, const glm::vec2& v )
{
    return out << v.x << " " << v.y;
}

inline std::ostream& operator<<( std::ostream& out, const glm::vec3& v )
{
    return out << v.x << " " << v.y << " " << v.z;
}

inline std::ostream& operator<<( std::ostream& out, const glm::vec4& v )
{
    return out << v.x << " " << v.y << " " << v.z << " " << v.w;
}

inline std::ostream& operator<<( std::ostream& out, const glm::mat4& v )
{
    return out << v[0] << "\n" << v[1] << "\n" << v[2] << "\n" << v[3];
}

inline std::istream& operator>>( std::istream& in, glm::vec2& v )
{
    return in >> v.x >> v.y;
}

inline std::istream& operator>>( std::istream& in, glm::vec3& v )
{
    return in >> v.x >> v.y >> v.z;
}

inline std::istream& operator>>( std::istream& in, glm::vec4& v )
{
    return in >> v.x >> v.y >> v.z >> v.w;
}
