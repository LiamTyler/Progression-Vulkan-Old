#pragma once

#include "glad/glad.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include "core/configuration.hpp"
#include <string>
#include <iostream>
#include <memory>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define UNUSED(x) (void)(x);

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

    inline std::ostream& operator <<(std::ostream& out, const glm::mat4& v) {
        return out << v[0] << "\n" << v[1] << "\n" << v[2] << "\n" << v[3];
    }

    inline std::istream& operator >>(std::istream& in, glm::vec2& v) {
        return in >> v.x >> v.y;
    }

	inline std::istream& operator >>(std::istream& in, glm::vec3& v) {
		return in >> v.x >> v.y >> v.z;
	}

	inline std::istream& operator >>(std::istream& in, glm::vec4& v) {
		return in >> v.x >> v.y >> v.z >> v.w;
	}

    inline glm::vec3 rotationToDirection(const glm::vec3& rotation, const glm::vec3& forward = glm::vec3(0, 0, -1)) {
        glm::mat4 rot(1);
        rot = glm::rotate(rot, rotation.z, glm::vec3(0, 0, 1));
        rot = glm::rotate(rot, rotation.y, glm::vec3(0, 1, 0));
        rot = glm::rotate(rot, rotation.x, glm::vec3(1, 0, 0));
        return glm::normalize(glm::vec3(rot * glm::vec4(forward, 0)));
    }

} // namespace Progression
