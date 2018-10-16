#include "graphics/skybox.h"
#include "core/image.h"
#include "core/resource_manager.h"

namespace Progression {

    Skybox::Skybox() :
        skyboxTextureID_(-1),
        cubeVao_(-1),
        cubeVbo_(-1)
    {
        shader_ = ResourceManager::GetShader("skybox");
    }

    Skybox::Skybox(
        const std::string& right,
        const std::string& left,
        const std::string& top,
        const std::string& bottom,
        const std::string& front,
        const std::string& back) :
        skyboxTextureID_(-1),
        cubeVao_(-1),
        cubeVbo_(-1)
    {
        shader_ = ResourceManager::GetShader("skybox");
        Load({ right, left, top, bottom, front, back });
    }

	Skybox::~Skybox() {
        if (skyboxTextureID_ != -1)
			glDeleteTextures(1, &skyboxTextureID_);
        if (cubeVbo_ != -1)
			glDeleteBuffers(1, &cubeVbo_);
        if (cubeVao_ != -1)
			glDeleteVertexArrays(1, &cubeVao_);
	}

    Skybox::Skybox(Skybox&& skybox) {
        *this = std::move(skybox);
    }

    Skybox& Skybox::operator=(Skybox&& skybox) {
        skyboxTextureID_ = skybox.skyboxTextureID_;
        cubeVao_ = skybox.cubeVao_;
        cubeVbo_ = skybox.cubeVbo_;
        skybox.skyboxTextureID_ = -1;
        skybox.cubeVao_ = -1;
        skybox.cubeVbo_ = -1;

        return *this;
    }

	bool Skybox::Load(const std::vector<std::string>& faces) {
		float skyboxVertices[] = {
			// positions          
			-1.0f,  1.0f, -1.0f,
			-1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f, -1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,

			-1.0f, -1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f,
			-1.0f, -1.0f,  1.0f,

			-1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f, -1.0f,
			 1.0f,  1.0f,  1.0f,
			 1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f,  1.0f,
			-1.0f,  1.0f, -1.0f,

			-1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f, -1.0f,
			 1.0f, -1.0f, -1.0f,
			-1.0f, -1.0f,  1.0f,
			 1.0f, -1.0f,  1.0f
		};

		glGenVertexArrays(1, &cubeVao_);
		glBindVertexArray(cubeVao_);
		glGenBuffers(1, &cubeVbo_);
		glBindBuffer(GL_ARRAY_BUFFER, cubeVbo_);
		glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices),
			skyboxVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray((*shader_)["vertex"]);
		glVertexAttribPointer((*shader_)["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);


		glGenTextures(1, &skyboxTextureID_);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID_);

		for (int i = 0; i < 6; i++) {
			Image img;
			if (!img.LoadImage(faces[i])) {
				std::cout << "Failed to load skybox texture: " << faces[i] << std::endl;
				return false;
			}

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA,
				img.Width(), img.Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.GetData());

		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		return true;
	}

	void Skybox::Render(const Camera& camera) {
        shader_->Enable();
		glm::mat4 P = camera.GetP();
		glm::mat4 RV = glm::mat4(glm::mat3(camera.GetV()));
		glUniformMatrix4fv((*shader_)["VP"], 1, GL_FALSE, glm::value_ptr(P * RV));
		glDepthMask(GL_FALSE);
		glBindVertexArray(cubeVao_);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID_);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glDepthMask(GL_TRUE);
	}

} // namespace Progression