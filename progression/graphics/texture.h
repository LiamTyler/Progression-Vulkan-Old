#pragma once

#include "core/image.h"

namespace Progression {

    class Texture {
    public:
        Texture(Image* image = nullptr, bool mipMapped = true, bool upload = true, bool free = true);
        ~Texture();

        Texture(const Texture& texture) = delete;
        Texture& operator=(const Texture& texture) = delete;
        Texture(Texture&& texture);
        Texture& operator=(Texture&& texture);

        // TODO: If uploaded once, and a change it made, detect if we can use subimage, or image
        void UploadToGPU(bool free = true);
        
        GLuint getGPUHandle() const { return gpuHandle_; }
        Image* getImage() const { return image_; }

    private:
        Image* image_;
        GLuint gpuHandle_;
        bool mipMapped_;
    };

} // namespace Progression