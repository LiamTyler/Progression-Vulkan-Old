#pragma once

#include "core/common.hpp"

namespace Progression { namespace graphics {

    void ToggleDepthBufferWriting(bool b);
    void ToggleBlending(bool b, GLenum srcFactor = GL_ONE, GLenum dstFactor = GL_ONE);
    void ToggleCulling(bool b, GLenum mode = GL_BACK);
    void ToggleDepthTesting(bool b);

    void Bind2DTexture(GLuint tex, GLuint uniformLocation, int unit);

    GLuint CreateVAO();

    GLuint Create2DTexture(
        int width,
        int height,
        GLenum internalFormat = GL_RGB,
        GLint minFilter = GL_NEAREST,
        GLint magFilter = GL_NEAREST
    );

    GLuint CreateFrameBuffer();
    GLuint CreateRenderBuffer(int width, int height, GLenum internalFormat = GL_DEPTH_COMPONENT);
    void AttachColorTexturesToFBO(std::initializer_list<GLuint> colorAttachments);
    void AttachRenderBufferToFBO(GLuint buffer, GLenum attachmentType = GL_DEPTH_ATTACHMENT);
    void FinalizeFBO();
    void BindFrameBuffer(GLuint fbo = 0);

    void SetClearColor(const glm::vec4& color);
    void SetClearColor(float r, float g, float b, float a);
    void Clear(GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


} } // namespace Progression::graphics
