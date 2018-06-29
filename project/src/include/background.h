#pragma once

#include "include/utils.h"
#include "include/skybox.h"
#include "include/camera.h"

namespace Progression {

	class Background {
	public:
		Background(const glm::vec4& c = glm::vec4(1), Skybox* sb = nullptr);

		void ClearAndRender(const Camera& camera);

		glm::vec4 GetColor() { return color_; }
		void SetColor(const glm::vec4& c) { color_ = c; }

	private:
		glm::vec4 color_;
		Skybox* skybox_;
	};

} // namespace Progression
