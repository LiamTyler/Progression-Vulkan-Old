#include "resource/texture2D.hpp"
#include "utils/logger.hpp"

namespace Progression {

    Texture2D::Texture2D() {
        glGenTextures(1, &gpuHandle_);
    }

    Texture2D::Texture2D(Image* img, const TextureUsageDesc& _desc, bool freeCPUCopy) :
        image(img),
        desc(_desc)
    {
        glGenTextures(1, &gpuHandle_);
        uploadToGPU(freeCPUCopy);
    }

    Texture2D::~Texture2D() {
        if (image)
            delete image;
        if (gpuHandle_ != (GLuint) -1)
            glDeleteTextures(1, &gpuHandle_);
    }

    Texture2D::Texture2D(Texture2D&& texture) {
        *this = std::move(texture);
    }

    Texture2D& Texture2D::operator=(Texture2D&& texture) {
        gpuHandle_     = std::move(texture.gpuHandle_);
        width_         = std::move(texture.width_);
        height_        = std::move(texture.height_);
        image          = std::move(texture.image);
        desc           = std::move(texture.desc);

        texture.image  = nullptr;
        texture.gpuHandle_ = (GLuint) -1;

        return *this;
    }

    // TODO: Find out if mipmaps need to be regenerated
    void Texture2D::uploadToGPU(bool freeCPUCopy) {
        if (!image)
            return;

        glBindTexture(GL_TEXTURE_2D, gpuHandle_);
        static const GLenum formats[] = { GL_RED, GL_RG, GL_RGB, GL_RGBA };

        // check to see if the image has a new size. If so, create new gpu image
        if (width_ != image->width() || height_ != image->height()) {
            width_ = image->width();
            height_ = image->height();

            glTexImage2D(
                    GL_TEXTURE_2D,
                    0,
                    desc.internalFormat,
                    width_,
                    height_,
                    0,
                    formats[image->numComponents()],
                    GL_UNSIGNED_BYTE,
                    image->pixels()
                    );
            if (desc.mipMapped)
                glGenerateMipmap(GL_TEXTURE_2D);
        } else { // otherwise just stream the data
            glTexSubImage2D(
                    GL_TEXTURE_2D,
                    0,
                    0,
                    0,
                    width_,
                    height_,
                    GL_UNSIGNED_BYTE,
                    formats[image->numComponents()],
                    image->pixels()
                    );
        }

        if (freeCPUCopy) {
            delete image;
            image = nullptr;
        }
    }

} // namespace Progression
