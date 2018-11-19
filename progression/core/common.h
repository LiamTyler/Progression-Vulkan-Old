#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "core/configuration.h"
#include <string>
#include <iostream>
#include <memory>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Progression {

    inline std::ostream& operator <<(std::ostream& out, const glm::vec2& v) {
        return out << v.x << " " << v.y;
    }

	inline std::ostream& operator <<(std::ostream& out, const glm::vec3& v) {
		return out << v.x << " " << v.y << " " << v.z;
	}

	inline std::ostream& operator <<(std::ostream& out, const glm::vec4& v) {
		return out << v.x << " " << v.y << " " << v.z << " " << v.w;
	}

	inline std::istream& operator >>(std::istream& in, glm::vec3& v) {
		return in >> v.x >> v.y >> v.z;
	}

	inline std::istream& operator >>(std::istream& in, glm::vec4& v) {
		return in >> v.x >> v.y >> v.z >> v.w;
	}

} // namespace Progression
