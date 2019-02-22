#include "graphics/shadow_map.hpp"
#include "graphics/graphics_api.hpp"

namespace Progression {

    ShadowMap::ShadowMap(Type _type, int _width, int _height) :
        type_(_type),
        width_(_width),
        height_(_height)
    {
        fbo_ = graphicsApi::createFramebuffer();
        glGenTextures(1, &depthTex_);

        if (type_ == Type::QUAD) {
            glBindTexture(GL_TEXTURE_2D, depthTex_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            float borderColor[] = { 1.0, 1.0, 1.0, 0.0 };
            glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, width_, height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex_, 0);
        } else {
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthTex_);
            for (unsigned int i = 0; i < 6; ++i)
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT32F, width_, height_, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTex_, 0);
        }

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        graphicsApi::checkFboCompleteness();
        graphicsApi::bindFramebuffer(0);
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
        type_ = std::move(map.type_);
        width_ = std::move(map.width_);
        height_ = std::move(map.height_);

        fbo_ = map.fbo_;
        depthTex_ = map.depthTex_;
        map.fbo_ = (GLuint)-1;
        map.depthTex_ = (GLuint)-1;

        return *this;
    }

    void ShadowMap::BindForWriting() const {
        graphicsApi::setViewport(width_, height_);
        graphicsApi::bindFramebuffer(fbo_);
        graphicsApi::clearDepthBuffer();
    }

} // namespace Progression
