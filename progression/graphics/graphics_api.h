#pragma once

#include "core/common.h"

namespace Progression { namespace graphics {

    void ToggleDepthBufferWriting(bool b);
    void ToggleBlending(bool b, GLenum srcFactor = GL_ONE, GLenum dstFactor = GL_ONE);
    void ToggleCulling(bool b, GLenum mode = GL_BACK);
    void ToggleDepthTesting(bool b);

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

    void SetClearColor(const glm::vec4& color);
    void SetClearColor(float r, float g, float b, float a);


} } // namespace Progression::graphics