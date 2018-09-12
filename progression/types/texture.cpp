#include "types/texture.h"

namespace Progression {

    Texture::Texture(Image* image, bool mipMapped, bool upload, bool free) :
        image_(image),
        mipMapped_(mipMapped)
    {
        glGenTextures(1, &gpuHandle_);
        if (upload)
            UploadToGPU(free);
    }

    Texture::~Texture() {
        if (image_)
            delete image_;
        if (gpuHandle_ != -1)
            glDeleteTextures(1, &gpuHandle_);
    }

    Texture::Texture(Texture&& texture) {
        *this = std::move(texture);
    }

    Texture& Texture::operator=(Texture&& texture) {
        image_ = texture.image_;
        gpuHandle_ = texture.gpuHandle_;

        texture.image_ = nullptr;
        texture.gpuHandle_ = -1;

        return *this;
    }

    void Texture::UploadToGPU(bool free) {
        glBindTexture(GL_TEXTURE_2D, gpuHandle_);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_->Width(), image_->Height(), 0, GL_RGBA, GL_FLOAT, image_->GetData());
        if (mipMapped_) {
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                GL_LINEAR_MIPMAP_LINEAR);
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        if (free) {
            delete image_;
            image_ = nullptr;
        }
    }

} // namespace Progression