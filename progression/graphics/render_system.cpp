#include "core/scene.hpp"
#include "core/window.hpp"
#include "utils/logger.hpp"
#include "graphics/render_system.hpp"
#include "graphics/shader.hpp"
#include "graphics/graphics_api.hpp"
#include "graphics/model_render_component.hpp"
#include "graphics/mesh.hpp"

namespace {

    using namespace Progression;

    enum ShaderNames {
        GBUFFER_PASS = 0,
        // SHADOWPASS_DIRECTIONAL_SPOT,
        // SHADOWPASS_POINT,
        LIGHTPASS_POINT,
        LIGHTPASS_SPOT,
        LIGHTPASS_DIRECTIONAL,
        BACKGROUND_OR_SKYBOX,
        POST_PROCESS,
        FLAT,
        TOTAL_SHADERS
    };

    const std::string shaderPaths[TOTAL_SHADERS][3] = {
        { "gbuffer.vert", "gbuffer.frag", "" },
        { "lightVolume.vert", "light_pass_point.frag", "" },
        { "lightVolume.vert", "light_pass_spot.frag", "" },
        { "quad.vert", "light_pass_directional.frag", "" },
        { "background.vert", "background.frag", "" },
        { "quad.vert", "post_process.frag", "" },
        { "flat.vert", "flat.frag", "" },
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

        void bindForWriting() {
            graphicsApi::bindFramebuffer(fbo);
            graphicsApi::clearColor(0, 0, 0, 0);
            graphicsApi::clearColorBuffer();
            graphicsApi::clearDepthBuffer();
        }

        void bindForReading(const Shader& shader) {
            graphicsApi::bind2DTexture(positionTex, shader.getUniform("gPosition"), 0);
            graphicsApi::bind2DTexture(normalTex,   shader.getUniform("gNormal"), 1);
            graphicsApi::bind2DTexture(diffuseTex,  shader.getUniform("gDiffuse"), 2);
            graphicsApi::bind2DTexture(specularTex, shader.getUniform("gSpecularExp"), 3);
            graphicsApi::bind2DTexture(emissiveTex, shader.getUniform("gEmissive"), 4);
        }
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

    void shadowPass(Scene* scene);
    void gBufferPass(Scene* scene);
    void lightingPass(Scene* scene);
    void backgroundPass(Scene* scene);
    void postProcessPass(Scene* scene);

    bool Init(const config::Config& config) {
        UNUSED(config);
        UNUSED(gbuffer);

        // load shaders
        for (int i = 0; i < TOTAL_SHADERS; ++i) {
            std::string vertPath = PG_RESOURCE_DIR "shaders/" + shaderPaths[i][0];
            std::string fragPath = "";
            std::string geomPath = "";
            if (shaderPaths[i][1] != "")
                fragPath = PG_RESOURCE_DIR "shaders/" + shaderPaths[i][1];
            if (shaderPaths[i][2] != "")
                geomPath = PG_RESOURCE_DIR "shaders/" + shaderPaths[i][2];

            if (!shaders[i].load(vertPath, fragPath, geomPath)) {
                LOG_ERR("Failed to load the shader files:", vertPath, fragPath, geomPath);
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
        graphicsApi::describeAttribute(0, 2, GL_FLOAT);

        // load cube data
        float skyboxVertices[] = {
            // positions          
            // back
            -1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,

            //front
            -1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,

            // right
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,

            // left
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,

            // top
            -1.0f,  1.0f, -1.0f,
            1.0f,   1.0f,  1.0f,
            1.0f,   1.0f, -1.0f,
            1.0f,   1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,

            // bottom
            -1.0f, -1.0f, -1.0f,
            1.0f,  -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f,  -1.0f, -1.0f,
            1.0f,  -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
        };
        
        cubeVao = graphicsApi::createVao();
        cubeVbo = graphicsApi::createBuffer();
        glBindBuffer(GL_ARRAY_BUFFER, cubeVbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);
        graphicsApi::describeAttribute(0, 3, GL_FLOAT);

        
        // create gbuffer
        gbuffer.fbo = graphicsApi::createFramebuffer();
        graphicsApi::Texture2DDesc desc;
        desc.width = Window::width();
        desc.height = Window::height();
        desc.internalFormat = GL_RGBA32F;
        gbuffer.positionTex = graphicsApi::create2DTexture(desc);
        gbuffer.normalTex   = graphicsApi::create2DTexture(desc);
        desc.internalFormat = GL_RGBA16F;
        gbuffer.specularTex = graphicsApi::create2DTexture(desc);
        desc.internalFormat = GL_RGB16F;
        gbuffer.emissiveTex = graphicsApi::create2DTexture(desc);
        desc.internalFormat = GL_RGB;
        gbuffer.diffuseTex  = graphicsApi::create2DTexture(desc);
        gbuffer.depthRbo = graphicsApi::createRenderbuffer(Window::width(), Window::height());
        graphicsApi::attachColorTextures({ gbuffer.positionTex, gbuffer.normalTex,
                                           gbuffer.diffuseTex, gbuffer.specularTex,
                                           gbuffer.emissiveTex });
        graphicsApi::attachDepthRbo(gbuffer.depthRbo);
        graphicsApi::checkFboCompleteness();

        // create postProcessBuffer
        postProcessBuffer.fbo = graphicsApi::createFramebuffer();
        desc.internalFormat = GL_RGBA16F;
        postProcessBuffer.hdrColorTex = graphicsApi::create2DTexture(desc);
        postProcessBuffer.depthRbo = graphicsApi::createRenderbuffer(Window::width(), Window::height());
        graphicsApi::attachColorTextures({ postProcessBuffer.hdrColorTex });
        graphicsApi::attachDepthRbo(postProcessBuffer.depthRbo);
        graphicsApi::checkFboCompleteness();

        // other options
        graphicsApi::toggleDepthTest(true);
        graphicsApi::toggleDepthWrite(true);


        return true;
    }

    void Free() {
        // quad and cube data
        graphicsApi::deleteVao(quadVao);
        graphicsApi::deleteVao(cubeVao);
        graphicsApi::deleteBuffer(quadVbo);
        graphicsApi::deleteBuffer(cubeVbo);

        // gbuffer
        graphicsApi::deleteTexture(gbuffer.positionTex);
        graphicsApi::deleteTexture(gbuffer.normalTex);
        graphicsApi::deleteTexture(gbuffer.diffuseTex);
        graphicsApi::deleteTexture(gbuffer.specularTex);
        graphicsApi::deleteTexture(gbuffer.emissiveTex);
        graphicsApi::deleteRenderbuffer(gbuffer.depthRbo);
        graphicsApi::deleteFramebuffer(gbuffer.fbo);

        // post process data
        graphicsApi::deleteTexture(postProcessBuffer.hdrColorTex);
        graphicsApi::deleteRenderbuffer(postProcessBuffer.depthRbo);
        graphicsApi::deleteFramebuffer(postProcessBuffer.fbo);

        // shaders
        for (int i = 0; i < TOTAL_SHADERS; ++i) {
            shaders[i].free();
        }
    }

    void render(Scene* scene) {
        shadowPass(scene);

        gBufferPass(scene);

        // write the result of the lighting pass to the post processing FBO
        graphicsApi::bindFramebuffer(postProcessBuffer.fbo);
        graphicsApi::clearColor(0, 0, 0, 0);
        graphicsApi::clearColorBuffer();
        graphicsApi::blitFboToFbo(gbuffer.fbo, postProcessBuffer.fbo, Window::width(), Window::height(), GL_DEPTH_BUFFER_BIT);

        lightingPass(scene);


        backgroundPass(scene);

        postProcessPass(scene);

    }

    void shadowPass(Scene* scene) {
        UNUSED(scene);
    }

    void gBufferPass(Scene* scene) {
        gbuffer.bindForWriting();

        graphicsApi::toggleDepthTest(true);
        graphicsApi::toggleDepthWrite(true);
        graphicsApi::toggleCulling(false);

        auto& shader = shaders[ShaderNames::GBUFFER_PASS];
		shader.enable();
        auto VP = scene->getCamera()->GetP() * scene->getCamera()->GetV();

        // TODO: fix this crappy iterating later
        const auto& gameObjects = scene->getGameObjects();
        for (const auto& obj : gameObjects) {
            auto mr = obj->GetComponent<ModelRenderer>();
            if (mr && mr->enabled) {
                glm::mat4 M   = obj->transform.GetModelMatrix();
                glm::mat4 N   = glm::transpose(glm::inverse(M));
                glm::mat4 MVP = VP * M;
                shader.setUniform("M", M);
                shader.setUniform("N", N);
                shader.setUniform("MVP", MVP);
                for (size_t i = 0; i < mr->materials.size(); ++i) {
                    const auto& mesh     = mr->model->meshes[i];
                    const auto& material = mr->materials[i];
                    graphicsApi::bindVao(mesh->vao);
                    shader.setUniform("kd", material->diffuse);
                    shader.setUniform("ks", material->specular);
                    shader.setUniform("ke", material->emissive);
                    shader.setUniform("specular", material->shininess);
                    if (material->diffuseTexture) {
                        shader.setUniform("textured", true);
                        graphicsApi::bind2DTexture(material->diffuseTexture->getGPUHandle(), shader.getUniform("diffuseTex"), 0);
                    } else {
                        shader.setUniform("textured", false);
                    }
                    glDrawElements(GL_TRIANGLES, mesh->getNumIndices(), GL_UNSIGNED_INT, 0);
                }
            }
        }
    }

    // TODO: Possible use tighter bounding light volumes, and look into using the stencil buffer
    //       to help not light pixels that intersect the volume only in screen space, not world
    void lightingPass(Scene* scene) {
        scene->sortLights();
        const auto& lights = scene->getLights();
        const auto* camera = scene->getCamera();
        auto VP = camera->GetP() * camera->GetV();

        graphicsApi::toggleBlending(true);
        graphicsApi::blendFunction(GL_ONE, GL_ONE);
        graphicsApi::toggleCulling(true);
        graphicsApi::cullFace(GL_BACK);
        graphicsApi::toggleDepthWrite(false);
        graphicsApi::toggleDepthTest(true);

        auto numPointLights       = scene->getNumPointLights();
        auto numSpotLights        = scene->getNumSpotLights();
        auto numDirectionalLights = scene->getNumDirectionalLights();
        unsigned int index = 0;

        // point lights
        {
            auto& shader = shaders[ShaderNames::LIGHTPASS_POINT];
            shader.enable();
            gbuffer.bindForReading(shader);
            graphicsApi::bindVao(cubeVao);
            shader.setUniform("cameraPos", camera->transform.position);
            for (unsigned int i = 0; i < numPointLights; ++i) {
                Light* l = lights[index + i];
                shader.setUniform("lightPos", l->transform.position);
                shader.setUniform("lightColor", l->color * l->intensity);
                shader.setUniform("lightRadiusSquared", l->radius * l->radius);

                glm::mat4 M(1);
                M = glm::translate(M, l->transform.position);
                M = glm::scale(M, glm::vec3(l->radius));
                auto MVP = VP * M;
                shader.setUniform("MVP", MVP);

                float distToCam = glm::length(camera->transform.position - l->transform.position);
                // if the camera is inside of the volume, then render the back faces instead
                if (distToCam < sqrtf(3.05f) * l->radius) {
                    graphicsApi::cullFace(GL_FRONT);
                    graphicsApi::toggleDepthTest(false);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    graphicsApi::cullFace(GL_BACK);
                    graphicsApi::toggleDepthTest(true);
                } else {
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
            index += numPointLights;
        }

        // spot lights
        {
            auto& shader = shaders[ShaderNames::LIGHTPASS_SPOT];
            shader.enable();
            gbuffer.bindForReading(shader);
            graphicsApi::bindVao(cubeVao);
            shader.setUniform("cameraPos", camera->transform.position);
            for (unsigned int i = 0; i < numSpotLights; ++i) {
                Light* l = lights[index + i];
                shader.setUniform("lightPos", l->transform.position);
                shader.setUniform("lightColor", l->color * l->intensity);
                shader.setUniform("lightRadiusSquared", l->radius * l->radius);
                shader.setUniform("lightInnerCutoff", glm::cos(l->innerCutoff));
                shader.setUniform("lightOuterCutoff", glm::cos(l->outerCutoff));
                glm::vec3 dir = rotationToDirection(l->transform.rotation);
                shader.setUniform("lightDir", dir);

                // create a bounding box around the center (not beginning) of the spot light
                Transform t = l->transform;
                t.position += 0.5f * l->radius * dir;
                float maxExtent = glm::tan(l->outerCutoff) * l->radius;
                t.scale = glm::vec3(maxExtent, maxExtent, 0.5f * l->radius);
                auto MVP = VP * t.GetModelMatrix();
                shader.setUniform("MVP", MVP);

                maxExtent = std::max(maxExtent, 0.5f * l->radius);
                float distToCam = glm::length(camera->transform.position - t.position);
                // if the camera is inside of the volume, then render the back faces instead
                if (distToCam < sqrtf(3.05) * maxExtent) {
                    graphicsApi::cullFace(GL_FRONT);
                    graphicsApi::toggleDepthTest(false);
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                    graphicsApi::cullFace(GL_BACK);
                    graphicsApi::toggleDepthTest(true);
                } else {
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }
            }
            index += numSpotLights;
        }

        graphicsApi::toggleCulling(false);
        graphicsApi::toggleDepthTest(false);
        // directional lights
        {
            auto& shader = shaders[ShaderNames::LIGHTPASS_DIRECTIONAL];
            shader.enable();
            gbuffer.bindForReading(shader);
            graphicsApi::bindVao(quadVao);
            shader.setUniform("cameraPos", camera->transform.position);
            for (unsigned int i = 0; i < numDirectionalLights; ++i) {
                Light* l = lights[index + i];

                glm::vec3 lightDir = rotationToDirection(l->transform.rotation);
                shader.setUniform("lightColor", l->color * l->intensity);
                shader.setUniform("lightDir", -lightDir);

                glDrawArrays(GL_TRIANGLES, 0, 6);
            }
        }

        graphicsApi::toggleBlending(false);
        graphicsApi::toggleDepthWrite(true);
        graphicsApi::toggleDepthTest(true);
    }

    void backgroundPass(Scene* scene) {
        Shader& shader = shaders[ShaderNames::BACKGROUND_OR_SKYBOX];
        shader.enable();
        graphicsApi::toggleDepthWrite(false);

        if (scene->skybox) {
            glm::mat4 P = scene->getCamera()->GetP();
            glm::mat4 RV = glm::mat4(glm::mat3(scene->getCamera()->GetV()));
            shader.setUniform("MVP", P * RV);
            shader.setUniform("skybox", true);
            graphicsApi::bindVao(cubeVao);
            graphicsApi::bindCubemap(scene->skybox->getGPUHandle(), shader.getUniform("cubeMap"), 0);
            glDrawArrays(GL_TRIANGLES, 0, 36);
        } else {
            shader.setUniform("MVP", glm::mat4(1));
            shader.setUniform("skybox", false);
            shader.setUniform("color", scene->backgroundColor);
            graphicsApi::bindVao(quadVao);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        graphicsApi::toggleDepthWrite(true);
    }

    void postProcessPass(Scene* scene) {
        // Bind and clear the main screen
        graphicsApi::bindFramebuffer(0);
        graphicsApi::setViewport(Window::width(), Window::height());
        graphicsApi::clearColor(0, 0, 0, 0);
        graphicsApi::clearColorBuffer();

        // Copy the post processing depth buffer to the main screen
        // graphicsApi::blitFboToFbo(postProcessBuffer.fbo, 0, Window::width(), Window::height(), GL_DEPTH_BUFFER_BIT);

        graphicsApi::toggleDepthWrite(false);
        graphicsApi::toggleDepthTest(false);

        // Draw the post processing color texture to screen, while performing
        // post processing effects, tone mapping, and gamma correction
        auto& shader = shaders[ShaderNames::POST_PROCESS];
        shader.enable();
        shader.setUniform("exposure", scene->getCamera()->renderOptions.exposure);
        graphicsApi::bind2DTexture(postProcessBuffer.hdrColorTex, shader.getUniform("originalColor"), 0);
        graphicsApi::bindVao(quadVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        graphicsApi::toggleDepthWrite(true);
        graphicsApi::toggleDepthTest(true);
    }


} } // namespace Progression::RenderSystem
