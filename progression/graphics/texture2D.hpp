#pragma once

#include "core/image.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

    class Texture2D : public NonCopyable {
    public:
        Texture2D();
        Texture2D(Image* image, bool free = true);
        ~Texture2D();


        Texture2D(Texture2D&& texture);
        Texture2D& operator=(Texture2D&& texture);

        bool Load(const std::string& fname, bool free = true);
        void UploadToGPU(bool free = true);
        void UpdateParameters();
        
        GLuint getGPUHandle() const { return gpuHandle_; }
        unsigned int getWidth() const { return width_; }
        unsigned int getHeight() const { return height_; }

        Image* image         = nullptr;
        GLint internalFormat = GL_RGB;
        GLint minFilter      = GL_LINEAR;
        GLint magFilter      = GL_LINEAR;
        GLint wrapModeS      = GL_REPEAT;
        GLint wrapModeT      = GL_REPEAT;
        bool mipMapped       = true;

    private:
        GLuint gpuHandle_ = -1;
        int width_        = 0;
        int height_       = 0;
    };

} // namespace Progression
