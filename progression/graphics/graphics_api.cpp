#include "graphics/graphics_api.h"

namespace Progression { namespace graphics {

    void ToggleDepthBufferWriting(bool b) {
        glDepthMask(b ? GL_TRUE : GL_FALSE);
    }

    void ToggleBlending(bool b, BlendType type) {
        if (b) {
            glEnable(GL_BLEND);
            if (type == BlendType::ADDITIVE)
                glBlendFunc(GL_ONE, GL_ONE);
        } else {
            glDisable(GL_BLEND);
        }
    }
    void ToggleCulling(bool b, CullType type) {
        if (b) {
            glEnable(GL_CULL_FACE);
            if (type == CullType::BACK) {
                glCullFace(GL_BACK);
            } else if (type == CullType::FRONT) {
                glCullFace(GL_FRONT);
            } else {
                glCullFace(GL_FRONT_AND_BACK);
            }
        } else {
            glDisable(GL_CULL_FACE);
        }
    }

    void ToggleDepthTesting(bool b) {
        if (b)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    GLuint Create2DTexture(int width, int height, TextureFormat internalFormat,
        Filter minFilter, Filter magFilter, unsigned char* data, TextureFormat dataFormat)
    {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

        return tex;
    }

    GLuint CreateFrameBuffer() {
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        return fbo;
    }

    void SetClearColor(const glm::vec4& color) {
        SetClearColor(color.r, color.g, color.b, color.a);
    }

    void SetClearColor(float r, float g, float b, float a) {
        glClearColor(r, b, g, a);
    }


} } // namespace Progression::graphics