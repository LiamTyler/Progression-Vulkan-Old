#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <string>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <cassert>
#include <unordered_map>

inline std::ostream& operator <<(std::ostream& out, const glm::vec3& v) {
    return out << v.x << " " << v.y << " " << v.z;
}

inline std::ostream& operator <<(std::ostream& out, const glm::vec4& v) {
    return out << v.x << " " << v.y << " " << v.z << " " << v.w;
}
