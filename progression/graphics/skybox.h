#pragma once

#include "core/common.h"
#include "graphics/shader.h"
#include "core/camera.h"

namespace Progression {

	class Skybox {
	public:
		Skybox(
            const std::string& right,
            const std::string& left,
            const std::string& top,
            const std::string& bottom,
            const std::string& front,
            const std::string& back);
		~Skybox();

        // TODO: consider copy / move pattern for all resources like this
        Skybox(const Skybox& skybox) = delete;
        Skybox& operator=(const Skybox& skybox) = delete;
        Skybox(Skybox&& skybox);
        Skybox& operator=(Skybox&& skybox);

		void Render(const Camera& camera);

		GLuint getGPUHandle() const { return skyboxTextureID_; }
		Shader* GetShader() const { return shader_; }

	private:
        bool Load(const std::vector<std::string>& faces);

		GLuint skyboxTextureID_;
		GLuint cubeVao_;
		GLuint cubeVbo_;
        Shader* shader_;
	};

} // namespace Progression