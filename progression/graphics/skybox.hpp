#pragma once

#include "core/common.hpp"
#include "graphics/shader.hpp"
#include "core/camera.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

	class Skybox : public NonCopyable {
	public:
        Skybox();
		~Skybox();
        Skybox(Skybox&& skybox);
        Skybox& operator=(Skybox&& skybox);

        bool Load(const std::vector<std::string>& faces);
        void Free();

		GLuint getGPUHandle() const { return skyboxTextureID_; }

	private:
		GLuint skyboxTextureID_;
	};

} // namespace Progression
