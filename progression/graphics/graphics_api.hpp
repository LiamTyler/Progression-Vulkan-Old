#pragma once

#include "core/common.hpp"

namespace Progression { namespace graphicsApi {

    // clear options
    
    /** \brief The color written into the color buffer when calling clearColorBuffer */
    void clearColor(float r, float g, float b, float a);

    /** \brief Clears the currently bound color buffer */
    void clearColorBuffer();

    /** \brief Clears the currently bound depth attachment */
    void clearDepthBuffer();


    // toggle options
    
    /** \brief Enable or disable depth testing */
    void toggleDepthTest(bool b);

    /** \brief Enable or disable writing to the depth buffer */
    void toggleDepthWrite(bool b);

    /** \brief Enable or disable face culling */
    void toggleCulling(bool b);

    /** \brief Enable or disable blending */
    void toggleBlending(bool b);


    // functions

    /** \brief Sets the viewport to have the specified width and height */
    void setViewport(const int width, const int height);

    /** \brief Blit's the data specified by mask of fromFbo to toFbo.
     *         When reading from the fromFbo, it will read whichever attachment is currently
     *         bound using glReadBuffer. This defaults to GL_BACK for the default fbo, otherwise
     *         it is GL_COLOR_ATTACHMENT0. When blitting to the toFbo, it will blit to whatever
     *         is bound using glDrawBuffers.
     *
     *         Mask needs to be GL_[COLOR,DEPTH,STENCIL]_BUFFER_BIT
     *
     *         Note: Need to test this for multiple color attachments
     */
    void blitFboToFbo(GLuint fromFbo, GLuint toFbo, int width, int height, GLbitfield mask);

    /** \brief Specify how to blend the incoming source color with the existing destination color */
    void blendFunction(GLenum srcFactor, GLenum dstFactor);

    /** \brief Specify which function causes the depth test to pass. OpenGL defaults to GL_LESS */
    void depthFunction(GLenum depthFunc);


    // vaos
    
    /** \brief Creates and automatically binds a vertex array object. */
    GLuint createVao();

    /** \brief Binds the given vao. */
    void bindVao(GLuint vao);

    /** \brief Enables an attribute at location, and describes the buffer data layout for that attribute. */
    void describeAttribute(GLuint location, GLint elements, GLenum type, int stride = 0, int offset = 0);

    /** \brief Delete the given vao. */
    void deleteVao(GLuint vao);

    // buffers

    /** \brief Creates a buffer, unbound. */
    GLuint createBuffer();

    /** \brief Creates as many buffers as specified by count, unbound. */
    void createBuffers(GLuint* buffers, int count);

    /** \brief Deletes the single buffer given. */
    void deleteBuffer(GLuint buffer);

    /** \brief Deletes the buffer(s) given. */
    void deleteBuffers(GLuint* buffers, int count);


    // textures

    /** \brief Binds the given texture, set the shader's uniform, and the active texture unit. */
    void bind2DTexture(GLuint tex, GLuint uniformLocation, int unit = 0);

    /** \brief Binds the given cubemap, set the shader's uniform, and the active texture unit. */
    void bindCubemap(GLuint tex, GLuint uniformLocation, int unit = 0);

    typedef struct Texture2DDesc {
        int width;
        int height;
        GLint internalFormat = GL_RGB;
        GLint minFilter = GL_NEAREST;
        GLint magFilter = GL_NEAREST;
        GLint wrapS = GL_CLAMP_TO_EDGE;
        GLint wrapT = GL_CLAMP_TO_EDGE;
        glm::vec4 borderColor = glm::vec4(1, 1, 1, 1);
    } Texture2DDesc;

    /** \brief Creates a texture based on the parameters in the texture descriptor struct. */
    GLuint create2DTexture(const Texture2DDesc& textureDesc);

    /** \brief Deletes the texture given. */
    void deleteTexture(GLuint textures);

    /** \brief Deletes the texture(s) given. */
    void deleteTextures(GLuint* textures, int count);


    // render buffers and framebuffers

    /** \brief Creates a render buffer with the given width, height, and internal format, and
     *         binds the buffer as well. */
    GLuint createRenderbuffer(int width, int height, GLenum internalFormat = GL_DEPTH_COMPONENT);

    /** \brief Deletes the rbo given. */
    void deleteRenderbuffer(GLuint rbo);

    /** \brief Creates a framebuffer and binds it automatically */
    GLuint createFramebuffer();

    /** \brief Deletes the framebuffer given. */
    void deleteFramebuffer(GLuint fbo);

    /** \brief Binds the given fbo. Default is the main screen buffer */
    void bindFramebuffer(GLuint fbo = 0);

    /** \brief Specifies which color textures are being drawn to. Each of these should
     *         be an output of the fragment shader. If none are supplied, then both the
     *         read and draw buffers are set to none (like for depth passes).
     */
    void attachColorTextures(std::initializer_list<GLuint> colorTextures);

    // NOTE: might need to specify between glFramebufferTexture and glFramebufferTexture2D?
    /** \brief Sets the depth attachment to be the given texture. */
    void attachDepthTexture(GLuint tex);

    /** \brief Sets the depth attachment to be the given rbo. */
    void attachDepthRbo(GLuint rbo);

    /** \brief Checks to see if the currently bound fbo is complete. Will log and error if not. */
    void checkFboCompleteness();


} } // namespace Progression::graphicsApi
