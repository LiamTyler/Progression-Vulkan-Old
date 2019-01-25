#include "graphics/skybox.hpp"
#include "core/image.hpp"
#include "core/resource_manager.hpp"

namespace Progression {

    Skybox::Skybox() : skyboxTextureID_(-1)
    {
    }

	Skybox::~Skybox() {
        Free();
	}

    Skybox::Skybox(Skybox&& skybox) {
        *this = std::move(skybox);
    }

    Skybox& Skybox::operator=(Skybox&& skybox) {
        skyboxTextureID_ = skybox.skyboxTextureID_;
        skybox.skyboxTextureID_ = -1;

        return *this;
    }

	bool Skybox::Load(const std::vector<std::string>& faces) {
		glGenTextures(1, &skyboxTextureID_);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxTextureID_);

		for (int i = 0; i < 6; i++) {
			Image img;
            if (!img.Load(faces[i], false)) {
                Free();
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

    void Skybox::Free() {
        if (skyboxTextureID_ != -1)
            glDeleteTextures(1, &skyboxTextureID_);
    }

} // namespace Progression
