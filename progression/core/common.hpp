#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "core/feature_defines.hpp"
#include "core/assert.hpp"
#include "core/unused.hpp"
#include "glm/ext.hpp"
#include "glm/glm.hpp"
#include <iostream>
#include <string>

#define ARRAY_COUNT( array ) ( static_cast< int >( sizeof( array ) / sizeof( array[0] ) ) )

#ifndef M_PI
#define M_PI 3.14159265358979323846
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
