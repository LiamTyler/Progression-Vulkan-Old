#include "graphics/graphics_api.hpp"
#include "utils/logger.hpp"
#include <vector>

namespace Progression { namespace graphicsApi {

    void clearColor(float r, float g, float b, float a) {
        glClearColor(r, b, g, a);
    }

    void clearColorBuffer() {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void clearDepthBuffer() {
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void toggleDepthTest(bool b) {
        if (b)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    void toggleDepthWrite(bool b) {
        glDepthMask(b ? GL_TRUE : GL_FALSE);
    }

    void toggleCulling(bool b) {
        if (b)
            glEnable(GL_CULL_FACE);
        else
            glDisable(GL_CULL_FACE);
    }

    void toggleBlending(bool b) {
        if (b)
            glEnable(GL_BLEND);
        else
            glDisable(GL_BLEND);
    }

    void setViewport(const int width, const int height) {
        glViewport(0, 0, width, height);
    }

    void blitFboToFbo(GLuint fromFbo, GLuint toFbo, int width, int height, GLbitfield mask) {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, fromFbo);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, toFbo);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, mask, GL_NEAREST);
    }

    void blendFunction(GLenum srcFactor, GLenum dstFactor) {
        glBlendFunc(srcFactor, dstFactor);
    }

    void depthFunction(GLenum depthFunc) {
        glDepthFunc(depthFunc);
    }


    GLuint createVaos() {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        return vao;
    }

    void deleteVao(GLuint vao) {
        glDeleteVertexArrays(1, &vao);
    }

    void bindVao(GLuint vao) {
        glBindVertexArray(vao);
    }

    void deleteBuffer(GLuint buffer) {
        glDeleteBuffers(1, &buffer);
    }

    GLuint createBuffer() {
        GLuint buffer;
        glGenBuffers(1, &buffer);
        return buffer;
    }

    void createBuffers(GLuint*& buffers, int count) {
        glGenBuffers(count, buffers);
    }

    void deleteBuffers(GLuint* buffers, int count) {
        glDeleteBuffers(count, buffers);
    }

    void bind2DTexture(GLuint tex, GLuint uniformLocation, int unit) {
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, tex);
        glUniform1i(uniformLocation, unit);
    }

    GLuint create2DTexture(const Texture2DDesc& desc) {
        GLuint tex;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, desc.internalFormat, desc.width, desc.height, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, desc.minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, desc.magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, desc.wrapS);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, desc.wrapT);
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &desc.borderColor.x);

        return tex;
    }

    void deleteTextures(GLuint* textures, int count) {
        glDeleteTextures(count, textures);
    }

    GLuint createRenderbuffer(int width, int height, GLenum internalFormat) {
        GLuint rbo;
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height);

        return rbo;
    }

    void deleteRenderbuffer(GLuint rbo) {
        glDeleteRenderbuffers(1, &rbo);
    }

    GLuint createFramebuffer() {
        GLuint fbo;
        glGenFramebuffers(1, &fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        return fbo;
    }

    void deleteFramebuffer(GLuint fbo) {
        glDeleteFramebuffers(1, &fbo);
    }

    void bindFramebuffer(GLuint fbo) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    }

    void attachColorTextures(std::initializer_list<GLuint> colorTextures) {
        if (!colorTextures.size()) {
            glDrawBuffer(GL_NONE);
            glReadBuffer(GL_NONE);
            return;
        }

        std::vector<GLuint> attachments;
        for (const auto& tex : colorTextures) {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachments.size(), GL_TEXTURE_2D, tex, 0);
            attachments.push_back(GL_COLOR_ATTACHMENT0 + attachments.size());
        }
        glDrawBuffers(attachments.size(), &attachments[0]);
    }

    void attachDepthTexture(GLuint texture) {
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture, 0);
    }

    void attachDepthRbo(GLuint rbo) {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    }

    void checkFboCompleteness() {
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            LOG_ERR("Frame buffer incomplete");
    }

} } // namespace Progression::graphicsApi
