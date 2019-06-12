#pragma once

#include "core/common.hpp"
#include "resource/image.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

    typedef struct TextureUsageDesc {
        GLint internalFormat = GL_SRGB;
        GLint minFilter      = GL_LINEAR;
        GLint magFilter      = GL_LINEAR;
        GLint wrapModeS      = GL_REPEAT;
        GLint wrapModeT      = GL_REPEAT;
        bool mipMapped       = true;
    } TextureUsageDesc;

    class Texture2D : public NonCopyable {
    public:
        Texture2D();
        Texture2D(Image* image, const TextureUsageDesc& desc, bool freeCPUCopy = true);
        ~Texture2D();

        Texture2D(Texture2D&& texture);
        Texture2D& operator=(Texture2D&& texture);

        void uploadToGPU(bool freeCPUCopy = true);
        
        GLuint gpuHandle() const { return gpuHandle_; }
        unsigned int width() const { return width_; }
        unsigned int height() const { return height_; }

        Image* image         = nullptr;
        TextureUsageDesc desc;

    private:
        GLuint gpuHandle_ = -1;
        int width_        = 0;
        int height_       = 0;
    };

} // namespace Progression
