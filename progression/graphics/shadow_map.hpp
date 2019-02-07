#pragma once

#include "core/common.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

    class ShadowMap {
    public:
        enum class Type {
            QUAD,
            CUBE
        };

        explicit ShadowMap(Type _type, int _width = 1024, int _height = 1024);
        ~ShadowMap();
        ShadowMap(ShadowMap&& map);
        ShadowMap& operator=(ShadowMap&& map);

        void BindForWriting() const;

        Type type() const { return type_; }
        int width() const { return width_; }
        int height() const { return height_; }
        int texture() const { return depthTex_; }

    private:
        Type type_;
        int width_;
        int height_;
        GLuint fbo_;
        GLuint depthTex_;
    };

} // namespace Progression 