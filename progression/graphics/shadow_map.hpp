#pragma once

#include "core/common.hpp"
#include "utils/noncopyable.hpp"

namespace Progression
{

class ShadowMap
{
public:
    enum class Type
    {
        QUAD,
        CUBE
    };

    explicit ShadowMap( Type _type, int _width = 1024, int _height = 1024 );
    ~ShadowMap();
    ShadowMap( ShadowMap&& map );
    ShadowMap& operator=( ShadowMap&& map );

    /** \brief Sets the viewport, binds the framebuffer, and clears the depth buffer */
    void BindForWriting() const;

    Type type() const
    {
        return type_;
    }
    int width() const
    {
        return width_;
    }
    int height() const
    {
        return height_;
    }
    int texture() const
    {
        return depthTex_;
    }

    /** \brief The light space matrices for this shadow map. These will be computed during the
     *         the shadow pass of the rendering, so that they are available later during the
     *         lighting phase. Directional and spot lights have 1 matrix, point lights have 6.
     */
    glm::mat4 LSMs[6];

private:
    Type type_;       ///< The type of shadow map (1 sided 'quad' or 6 sided 'cube')
    int width_;       ///< The width of depthTex_
    int height_;      ///< The height of depthTex_
    GLuint fbo_;      ///< The framebuffer that has the depthTex_ as its depth texture
    GLuint depthTex_; ///< The depth texture storing the shadow mapping information
};

} // namespace Progression
