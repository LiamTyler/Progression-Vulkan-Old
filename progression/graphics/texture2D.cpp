#include "graphics/texture2D.hpp"
#include "utils/logger.hpp"

namespace Progression {

    Texture2D::Texture2D() {
        glGenTextures(1, &gpuHandle_);
    }

    Texture2D::Texture2D(Image* img, bool free) : image(img)
    {
        glGenTextures(1, &gpuHandle_);
        UploadToGPU(free);
    }

    Texture2D::~Texture2D() {
        if (image)
            delete image;
        if (gpuHandle_ != -1)
            glDeleteTextures(1, &gpuHandle_);
    }

    Texture2D::Texture2D(Texture2D&& texture) {
        *this = std::move(texture);
    }

    Texture2D& Texture2D::operator=(Texture2D&& texture) {
        image          = std::move(texture.image);
        internalFormat = std::move(texture.internalFormat);
        minFilter      = std::move(texture.minFilter);
        magFilter      = std::move(texture.magFilter);
        wrapModeS      = std::move(texture.wrapModeS);
        wrapModeT      = std::move(texture.wrapModeT);
        mipMapped      = std::move(texture.mipMapped);
        gpuHandle_     = std::move(texture.gpuHandle_);
        width_         = std::move(texture.width_);
        height_        = std::move(texture.height_);

        texture.image  = nullptr;
        texture.gpuHandle_ = -1;

        return *this;
    }

    bool Texture2D::Load(const std::string& fname, bool free) {
        auto img = new Image;
        if (!img->Load(fname)) {
            delete img;
            LOG_ERR("Failed to load the image for texture:", fname);
            return false;
        }
        image = img;
        UploadToGPU(free);

        return true;
    }

    // TODO: Find out if mipmaps need to be regenerated
    void Texture2D::UploadToGPU(bool free) {
        if (!image)
            return;

        glBindTexture(GL_TEXTURE_2D, gpuHandle_);

        // check to see if the image has a new size. If so, create new gpu image
        if (width_ != image->Width() || height_ != image->Height()) {
            width_ = image->Width();
            height_ = image->Height();
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, image->Width(), image->Height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, image->GetData());
            if (mipMapped)
                glGenerateMipmap(GL_TEXTURE_2D);
            UpdateParameters();
        } else { // otherwise just stream the data
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_UNSIGNED_BYTE, GL_RGBA, image->GetData());
        }

        if (free) {
            delete image;
            image = nullptr;
        }
    }

    void Texture2D::UpdateParameters() {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapModeS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapModeT);
    }

} // namespace Progression
