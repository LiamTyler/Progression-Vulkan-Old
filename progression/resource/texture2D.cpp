#include "resource/texture2D.hpp"
#include "utils/logger.hpp"

namespace Progression {

    Texture2D::Texture2D() :
        Resource("")
    {
        glGenTextures(1, &gpuHandle_);
    }

    Texture2D::Texture2D(const std::string& _name, const TextureMetaData& data) :
        Resource(_name),
        metaData(data)
    {
        glGenTextures(1, &gpuHandle_);
        uploadToGPU();
    }

    Texture2D::~Texture2D() {
        if (metaData.image)
            delete metaData.image;
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
        metaData       = std::move(texture.metaData);

        texture.metaData.image  = nullptr;
        texture.gpuHandle_ = (GLuint) -1;

        return *this;
    }

    bool Texture2D::load() {
        Image* img = new Image;
        if (!img->load(metaData.file.filename)) {
            LOG_ERR("Could not load texture image: ", metaData.file.filename);
            delete img;
            return false;
        }

        if (metaData.image)
            delete metaData.image;
        metaData.image = img;
        uploadToGPU();

        return true;
    }

    // TODO: Find out if mipmaps need to be regenerated
    void Texture2D::uploadToGPU() {
        Image* image = metaData.image;
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
                    metaData.internalFormat,
                    width_,
                    height_,
                    0,
                    formats[image->numComponents() - 1],
                    GL_UNSIGNED_BYTE,
                    image->pixels()
                    );
            if (metaData.mipMapped)
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
                    formats[image->numComponents() - 1],
                    image->pixels()
                    );
        }

        if (metaData.freeCPUCopy) {
            delete image;
            metaData.image = nullptr;
        }
    }

} // namespace Progression
