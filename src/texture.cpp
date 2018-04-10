#include "include/texture.h"
#include "include/image.h"

Texture::Texture() : Texture("") {}

Texture::Texture(const std::string& fname) : Texture(fname, false) {}

Texture::Texture(const std::string& fname, bool mip_mapped) {
    filename_ = fname;
    mip_mapped_ = mip_mapped;
    gpu_handle_ = LoadTexture();
}

GLuint Texture::LoadTexture() {
    Image image;
    if (!image.LoadImage(filename_)) {
        std::cerr << "Failed to load tex: " << filename_ << std::endl;
        return -1;
    }

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.Width(), image.Height(), 0, GL_RGBA,
            GL_FLOAT, image.GetData());
    if (mip_mapped_) {
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                GL_LINEAR_MIPMAP_LINEAR);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return tex;
}
