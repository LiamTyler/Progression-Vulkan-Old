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
		void Render(const Camera& camera);

		GLuint getGPUHandle() const { return skyboxTextureID_; }
		std::shared_ptr<Shader> GetShader() const { return shader_; }

	private:
		GLuint skyboxTextureID_;
		GLuint cubeVao_;
		GLuint cubeVbo_;
        std::shared_ptr<Shader> shader_;
	};

} // namespace Progression
