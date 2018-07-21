#pragma once

#include "glad/glad.h"
#include <SDL.h>
#include <SDL_opengl.h>

#include "glm/glm.hpp"
#include "glm/ext.hpp"
#include <string>
#include <iostream>

namespace Progression {

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