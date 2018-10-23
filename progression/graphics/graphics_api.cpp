#include "graphics/graphics_api.h"
#include <vector>

namespace Progression { namespace graphics {

    void ToggleDepthBufferWriting(bool b) {
        glDepthMask(b ? GL_TRUE : GL_FALSE);
    }

    void ToggleBlending(bool b, GLenum srcFactor, GLenum dstFactor) {
        if (b) {
            glEnable(GL_BLEND);
            glBlendFunc(srcFactor, dstFactor);
        } else {
            glDisable(GL_BLEND);
        }
    }
    void ToggleCulling(bool b, GLenum mode) {
        if (b) {
            glEnable(GL_CULL_FACE);
            glCullFace(mode);
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

    GLuint CreateVAO() {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        return vao;
    }

    void Bind2DTexture(GLuint tex, GLuint uniformLocation, int unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(uniformLocation, unit);
    }

    GLuint Create2DTexture(int width, int height, GLenum internalFormat, GLint minFilter, GLint magFilter) {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        return tex;
    }

    GLuint CreateFrameBuffer() {
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        return fbo;
    }

    void AttachColorTexturesToFBO(std::initializer_list<GLuint> colorAttachments) {
        std::vector<GLuint> attachments;
        for (const auto& tex : colorAttachments) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachments.size(), GL_TEXTURE_2D, tex, 0);
            attachments.push_back(GL_COLOR_ATTACHMENT0 + attachments.size());
        }
        glDrawBuffers(attachments.size(), &attachments[0]);
    }

    GLuint CreateRenderBuffer(int width, int height, GLenum internalFormat) {
        GLuint rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height);

        return rbo;
    }

    void AttachRenderBufferToFBO(GLuint buffer, GLenum attachmentType) {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, attachmentType, GL_RENDERBUFFER, buffer);
    }

    void FinalizeFBO() {
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "frame buffer incomplete" << std::endl;
    }

    void BindFrameBuffer(GLuint fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    void SetClearColor(const glm::vec4& color) {
        SetClearColor(color.r, color.g, color.b, color.a);
    }

    void SetClearColor(float r, float g, float b, float a) {
        glClearColor(r, b, g, a);
    }

    void Clear(GLbitfield mask) {
        glClear(mask);
    }

} } // namespace Progression::graphics