#pragma once

#include "include/utils.h"

class Texture {
    public:
        Texture();
        Texture(const std::string& fname);
        Texture(const std::string& fname, bool mip_mapped);

        std::string GetName() { return filename_; }
        bool IsMipMapped() { return mip_mapped_; }
        GLuint GetHandle() { return gpu_handle_; }

    protected:
        GLuint LoadTexture();

        std::string filename_;
        GLuint gpu_handle_;
        bool mip_mapped_;
};
