#pragma once

#include "core/common.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

    class ShadowMap {
    public:
        ShadowMap(int _width = 1024, int _height = 1024);
        ~ShadowMap();
        ShadowMap(ShadowMap&& map);
        ShadowMap& operator=(ShadowMap&& map);

        void BindForWriting() const;

        int width() const { return width_; }
        int height() const { return height_; }
        int texture() const { return depthTex_; }

    private:
        int width_;
        int height_;
        GLuint fbo_;
        GLuint depthTex_;
    };

} // namespace Progression 