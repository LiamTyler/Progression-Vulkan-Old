#include "graphics/render_system.hpp"
#include "graphics/shader.hpp"
#include "graphics/graphics_api.hpp"
#include "utils/logger.hpp"

namespace {

    using namespace Progression;

    enum ShaderNames {
        GBUFFER_PASS = 0,
        // SHADOWPASS_DIRECTIONAL_SPOT,
        // SHADOWPASS_POINT,
        // LIGHTPASS_DIRECTIONAL,
        // LIGHTPASS_POINT,
        // LIGHTPASS_SPOT,
        // BACKGROUND_OR_SKYBOX,
        // POST_PROCESS,
        // DEBUG_FLAT,
        TOTAL_SHADERS
    };

    const std::string shaderPaths[TOTAL_SHADERS][3] = {
        { "default.vert", "gbuffer.frag", "" },
    };

    Shader shaders[TOTAL_SHADERS];

    struct GBuffer {
        GLuint fbo;
        GLuint positionTex;
        GLuint normalTex;
        GLuint diffuseTex;
        GLuint specularTex;
        GLuint ambientTex;
        GLuint emissiveTex;
        GLuint depthRbo;
    } gbuffer;

    struct PostProcessBuffer {
        GLuint fbo;
        GLuint hdrColorTex;
        GLuint depthRbo;
    } postProcessBuffer;

    GLuint quadVao;
    GLuint quadVbo;

    GLuint cubeVao;
    GLuint cubeVbo;

} // namespace anonymous

namespace Progression { namespace RenderSystem {

    void shadowPass(const Camera& camera) {

    }

    void gBufferPass(const Camera& camera) {

    }

    void lightingPass(const Camera& camera) {

    }

    void postProcessPass(const Camera& camera) {

    }

    bool Init(const config::Config& config) {
        UNUSED(config);
        UNUSED(cubeVao);
        UNUSED(cubeVbo);
        UNUSED(postProcessBuffer);
        UNUSED(gbuffer);

        // load shaders
        for (int i = 0; i < TOTAL_SHADERS; ++i) {
            if (!shaders[i].load(shaderPaths[i][0], shaderPaths[i][1], shaderPaths[i][2])) {
                LOG_ERR("Failed to load the shader files:", shaderPaths[i][0], shaderPaths[i][1], shaderPaths[i][2]);
                exit(EXIT_FAILURE);
            }
        }

        // load quad data
        float quadVerts[] = {
            -1, 1,
            -1, -1,
            1, -1,

            -1, 1,
            1, -1,
            1, 1
        };

        quadVao = graphicsApi::createVao();
        quadVbo = graphicsApi::createBuffer();
        glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // load cube data
        
        // create gbuffer

        // create postProcessBuffer


        return true;
    }

    void Free() {
    }

    void render(Scene* scene) {
        shadowPass(
    }

} } // namespace Progression::RenderSystem
