#include "graphics/shadow_map.hpp"
#include "graphics/graphics_api.hpp"

namespace Progression {

    ShadowMap::ShadowMap(int _width, int _height) :
        width_(_width),
        height_(_height)
    {
        fbo_ = graphics::CreateFrameBuffer();
        glGenTextures(1, &depthTex_);
        glBindTexture(GL_TEXTURE_2D, depthTex_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0, 1.0, 1.0, 0.0 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width_, height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex_, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        graphics::BindFrameBuffer();
    }

    ShadowMap::~ShadowMap() {
        if (fbo_ != (GLuint)-1)
            glDeleteFramebuffers(1, &fbo_);
        if (depthTex_ != (GLuint)-1)
            glDeleteTextures(1, &depthTex_);
    }

    ShadowMap::ShadowMap(ShadowMap&& map) {
        *this = std::move(map);
    }

    ShadowMap& ShadowMap::operator=(ShadowMap&& map) {
        width_ = std::move(map.width_);
        height_ = std::move(map.height_);

        fbo_ = map.fbo_;
        depthTex_ = map.depthTex_;
        map.fbo_ = (GLuint)-1;
        map.depthTex_ = (GLuint)-1;

        return *this;
    }

    void ShadowMap::BindForWriting() const {
        glViewport(0, 0, width_, height_);
        graphics::BindFrameBuffer(fbo_);
        graphics::Clear(GL_DEPTH_BUFFER_BIT);
    }

} // namespace Progression
