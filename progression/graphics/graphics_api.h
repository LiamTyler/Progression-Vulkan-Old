#pragma once

#include "core/common.h"

namespace Progression { namespace graphics {

    enum class BlendType {
        ADDITIVE
    };

    enum class CullType {
        FRONT,
        BACK,
        FRONT_AND_BACK
    };

    enum TextureFormat {
        RGB_CLAMPED = GL_RGB,
        RGBA_CLAMPED = GL_RGBA,
        RGB_32F = GL_RGB32F,
        RGBA_32F = GL_RGBA32F,
    };

    enum Filter {
        NEAREST = GL_NEAREST,
        LINEAR = GL_LINEAR
    };

    void ToggleDepthBufferWriting(bool b);
    void ToggleBlending(bool b, BlendType type = BlendType::ADDITIVE);
    void ToggleCulling(bool b, CullType type = CullType::BACK);
    void ToggleDepthTesting(bool b);

    GLuint Create2DTexture(
        int width,
        int height,
        TextureFormat internalFormat = TextureFormat::RGB_CLAMPED,
        Filter minFilter = Filter::NEAREST,
        Filter magFilter = Filter::NEAREST,
        unsigned char* data = nullptr,
        TextureFormat dataFormat = TextureFormat::RGBA_CLAMPED
    );

    GLuint CreateFrameBuffer();

    void SetClearColor(const glm::vec4& color);
    void SetClearColor(float r, float g, float b, float a);


} } // namespace Progression::graphics