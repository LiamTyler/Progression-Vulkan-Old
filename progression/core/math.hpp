#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE_
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <iostream>

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
