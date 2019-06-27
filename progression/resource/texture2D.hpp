#pragma once

#include "core/common.hpp"
#include "resource/image.hpp"
#include "utils/noncopyable.hpp"
#include "resource/resource.hpp"

namespace Progression {

    typedef struct TextureMetaData {
        TimeStampedFile file = TimeStampedFile();
        Image* image         = nullptr;
        GLint internalFormat = GL_SRGB;
        GLint minFilter      = GL_LINEAR;
        GLint magFilter      = GL_LINEAR;
        GLint wrapModeS      = GL_REPEAT;
        GLint wrapModeT      = GL_REPEAT;
        bool mipMapped       = true;
        bool freeCPUCopy     = true;
    } TextureUsageDesc;

    class Texture2D : public NonCopyable, public Resource {
    public:
        Texture2D();
        Texture2D(const std::string& name, const TextureMetaData& data);
        ~Texture2D();

        Texture2D(Texture2D&& texture);
        Texture2D& operator=(Texture2D&& texture);

        bool load();
        void uploadToGPU();
        
        GLuint gpuHandle() const { return gpuHandle_; }
        unsigned int width() const { return width_; }
        unsigned int height() const { return height_; }

        TextureMetaData metaData;

    private:
        GLuint gpuHandle_ = -1;
        int width_        = 0;
        int height_       = 0;
    };

} // namespace Progression
