#include "graphics/render_system.hpp"
#include "core/scene.hpp"
#include "core/window.hpp"
#include "graphics/graphics_api.hpp"
#include "resource/shader.hpp"
#include "utils/logger.hpp"
#include "components/model_renderer_component.hpp"
#include "core/ecs.hpp"
#include "graphics/lights.hpp"
#include "resource/material.hpp"
#include "resource/model.hpp"
// #include "resource/texture.hpp"
//#include "graphics/shadow_map.hpp"
#include <array>
#include "graphics/pg_to_opengl_types.hpp"

using namespace Progression;
using namespace Gfx;

enum ShaderNames
{
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

static const std::string s_shaderPaths[TOTAL_SHADERS][3] = {
    { "gbuffer.vert", "gbuffer.frag", "" },
    //{ "shadow.vert", "shadow.frag", "" },
    //{ "shadow.vert", "pointShadow.frag", "pointShadow.geom" },
    { "lightVolume.vert", "light_pass_point.frag", "" },
    { "lightVolume.vert", "light_pass_spot.frag", "" },
    { "quad.vert", "light_pass_directional.frag", "" },
    { "background.vert", "background.frag", "" },
    { "quad.vert", "post_process.frag", "" },
    { "flat.vert", "flat.frag", "" },
};

static std::unordered_map< std::string, Gfx::Sampler > s_samplers;

namespace Progression
{
namespace RenderSystem
{

    Gfx::Sampler* AddSampler( const std::string& name, Gfx::SamplerDescriptor& desc )
    {
        auto it = s_samplers.find( name );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }
        else
        {
            s_samplers[name] = Gfx::Sampler::Create( desc );
            return &s_samplers[name];
        }
    }

    Gfx::Sampler* GetSampler( const std::string& name )
    {
        auto it = s_samplers.find( name );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }

        return nullptr;
    }

} // namespace RenderSystem
} // namespace Progression

static Shader s_shaders[TOTAL_SHADERS];

struct GBuffer
{
    RenderPass renderPass;
    Gfx::Texture positionTex;
    Gfx::Texture normalTex;
    Gfx::Texture diffuseTex;
    Gfx::Texture specularTex;
    Gfx::Texture emissiveTex;
    Gfx::Texture depthRbo;

    void BindForReading( Shader& shader );
};

struct PostProcessBuffer
{
    RenderPass renderPass;
    Gfx::Texture hdrColorTex;
};

static Window* s_window;
static Viewport s_windowViewport;
static RenderPass s_screenRenderPass;
static GBuffer s_gbuffer;
static PostProcessBuffer s_postProcessBuffer;
static Buffer s_quadVertexBuffer;
static Buffer s_cubeVertexBuffer;
static Pipeline s_gbufferPipeline;
static Pipeline s_directionalLightPipeline;
static Pipeline s_backgroundPipeline;
static Pipeline s_postProcessPipeline;

void GBuffer::BindForReading( Shader& shader )
{
    shader.BindTexture( positionTex, "gPosition",    0 );
    shader.BindTexture( normalTex,   "gNormal",      1 );
    shader.BindTexture( diffuseTex,  "gDiffuse",     2 );
    shader.BindTexture( specularTex, "gSpecularExp", 3 );
    shader.BindTexture( emissiveTex, "gEmissive",    4 );

    Sampler* nearestSampler = RenderSystem::GetSampler( "nearest" );
    nearestSampler->Bind( 0 );
    nearestSampler->Bind( 1 );
    nearestSampler->Bind( 2 );
    nearestSampler->Bind( 3 );
    nearestSampler->Bind( 4 );
}

// helper functions
// void depthRender(Scene* scene, Shader& shader, const glm::mat4& LSM) {
//     LOG("depth render");
//     const auto& gameObjects = scene->getGameObjects();
//     for (const auto& obj : gameObjects) {
//         auto mr = obj->GetComponent<ModelRenderer>();
//         if (mr && mr->enabled) {
//             glm::mat4 M   = obj->transform.GetModelMatrix();
//             glm::mat4 MVP = LSM * M;
//             shader.setUniform("MVP", MVP);
//             for (size_t i = 0; i < mr->model->meshes.size(); ++i) {
//                 const auto& mesh = mr->model->meshes[i];
//                 graphicsApi::bindVao(mesh->vao);
//                 glDrawElements(GL_TRIANGLES, mesh->getNumIndices(), GL_UNSIGNED_INT, 0);
//             }
//         }
//     }
// }

namespace Progression
{
namespace RenderSystem
{

    void InitSamplers();
    // void shadowPass(Scene* scene);
    void GBufferPass( Scene* scene );
    void LightingPass( Scene* scene );
    void BackgroundPass( Scene* scene );
    void PostProcessPass( Scene* scene );

    bool Init()
    {
        InitSamplers();
        s_window = GetMainWindow();

        s_windowViewport.x = 0;
        s_windowViewport.y = 0;
        s_windowViewport.width = s_window->Width();
        s_windowViewport.height = s_window->Height();
        SetViewport( s_windowViewport );

        // load shaders
        for ( int i = 0; i < TOTAL_SHADERS; ++i )
        {
            ShaderCreateInfo info;
            info.vertex   = PG_RESOURCE_DIR "shaders/" + s_shaderPaths[i][0];
            info.fragment = PG_RESOURCE_DIR "shaders/" + s_shaderPaths[i][1];
            if ( s_shaderPaths[i][2] != "" )
                info.geometry = PG_RESOURCE_DIR "shaders/" + s_shaderPaths[i][2];

            if ( !s_shaders[i].Load( &info ) )
            {
                LOG_ERR( "Failed to load the render system shader: ", i );
                return false;
            }
        }



        // load quad data
        float quadVerts[] =
        {
            -1, 1, 0,  -1, -1, 0,   1, -1, 0,
            -1, 1, 0,   1, -1, 0,   1,  1, 0
        };
        
        std::array< VertexAttributeDescriptor, 1 > quadVertexDesc;
        quadVertexDesc[0].binding  = 0;
        quadVertexDesc[0].location = 0;
        quadVertexDesc[0].count    = 3;
        quadVertexDesc[0].format   = BufferDataType::FLOAT32;
        quadVertexDesc[0].offset   = 0;

        s_quadVertexBuffer = Buffer::Create( quadVerts, sizeof( quadVerts ), BufferType::VERTEX, BufferUsage::STATIC );

        // load cube data
        float skyboxVertices[] =
        {
            // positions
            // back
            -1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,

            // front
            -1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,

            // right
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,

            // left
            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,

            // top
            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,

            // bottom
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,
        };


        std::array< VertexAttributeDescriptor, 1 > cubeVertexDesc;
        cubeVertexDesc[0].binding  = 0;
        cubeVertexDesc[0].location = 0;
        cubeVertexDesc[0].count    = 3;
        cubeVertexDesc[0].format   = BufferDataType::FLOAT32;
        cubeVertexDesc[0].offset   = 0;

        s_cubeVertexBuffer = Buffer::Create( skyboxVertices, sizeof( skyboxVertices ), BufferType::VERTEX, BufferUsage::STATIC );

        // gbuffer render pass
        RenderPassDescriptor gBufferRenderPassDesc;

        ImageDescriptor texDesc;
        texDesc.width     = s_window->Width();
        texDesc.height    = s_window->Height();
        texDesc.type      = ImageType::TYPE_2D;
        texDesc.format    = PixelFormat::R32_G32_B32_A32_Float;
        
        s_gbuffer.positionTex = Gfx::Texture::Create( texDesc, nullptr );
        s_gbuffer.normalTex   = Gfx::Texture::Create( texDesc, nullptr );
        texDesc.format = PixelFormat::R8_G8_B8_Uint; // TODO: should this be sRGB?
        s_gbuffer.diffuseTex  = Gfx::Texture::Create( texDesc, nullptr );
        texDesc.format = PixelFormat::R16_G16_B16_A16_Float;
        s_gbuffer.specularTex = Gfx::Texture::Create( texDesc, nullptr );
        texDesc.format = PixelFormat::R16_G16_B16_Float;
        s_gbuffer.emissiveTex = Gfx::Texture::Create( texDesc, nullptr );
        texDesc.format = PixelFormat::DEPTH32_Float;
        s_gbuffer.depthRbo  = Gfx::Texture::Create( texDesc, nullptr );

        gBufferRenderPassDesc.colorAttachmentDescriptors[0].texture = &s_gbuffer.positionTex;
        gBufferRenderPassDesc.colorAttachmentDescriptors[1].texture = &s_gbuffer.normalTex;
        gBufferRenderPassDesc.colorAttachmentDescriptors[2].texture = &s_gbuffer.diffuseTex;
        gBufferRenderPassDesc.colorAttachmentDescriptors[3].texture = &s_gbuffer.specularTex;
        gBufferRenderPassDesc.colorAttachmentDescriptors[4].texture = &s_gbuffer.emissiveTex;
        gBufferRenderPassDesc.depthAttachmentDescriptor.texture = &s_gbuffer.depthRbo;

        s_gbuffer.renderPass = RenderPass::Create( gBufferRenderPassDesc );

        // postprocess render pass
        RenderPassDescriptor postProcessRenderPassDesc;

        texDesc.format = PixelFormat::R16_G16_B16_A16_Float;
        s_postProcessBuffer.hdrColorTex = Gfx::Texture::Create( texDesc, nullptr );
        postProcessRenderPassDesc.colorAttachmentDescriptors[0].texture = &s_postProcessBuffer.hdrColorTex;
        postProcessRenderPassDesc.colorAttachmentDescriptors[0].loadAction = LoadAction::CLEAR;
        postProcessRenderPassDesc.depthAttachmentDescriptor.texture = &s_gbuffer.depthRbo;
        postProcessRenderPassDesc.depthAttachmentDescriptor.loadAction = LoadAction::LOAD;

        s_postProcessBuffer.renderPass = RenderPass::Create( postProcessRenderPassDesc );

        RenderPassDescriptor mainScreenDescriptor;
        mainScreenDescriptor.colorAttachmentDescriptors[0].loadAction = LoadAction::CLEAR;
        mainScreenDescriptor.depthAttachmentDescriptor.loadAction     = LoadAction::DONT_CARE;
        s_screenRenderPass = RenderPass::Create( mainScreenDescriptor );

        // gbuffer pass pipeline
        PipelineDescriptor gbufferPipelineDesc;

        std::array< VertexAttributeDescriptor, 3 > meshAttribDescs;
        meshAttribDescs[0].binding  = 0;
        meshAttribDescs[0].location = 0;
        meshAttribDescs[0].count    = 3;
        meshAttribDescs[0].format   = BufferDataType::FLOAT32;
        meshAttribDescs[0].offset   = 0;

        meshAttribDescs[1].binding  = 1;
        meshAttribDescs[1].location = 1;
        meshAttribDescs[1].count    = 3;
        meshAttribDescs[1].format   = BufferDataType::FLOAT32;
        meshAttribDescs[1].offset   = 0;

        meshAttribDescs[2].binding  = 2;
        meshAttribDescs[2].location = 2;
        meshAttribDescs[2].count    = 2;
        meshAttribDescs[2].format   = BufferDataType::FLOAT32;
        meshAttribDescs[2].offset   = 0;

        gbufferPipelineDesc.numVertexDescriptors = 3;
        gbufferPipelineDesc.vertexDescriptors    = &meshAttribDescs[0];
        gbufferPipelineDesc.windingOrder         = WindingOrder::COUNTER_CLOCKWISE;
        gbufferPipelineDesc.cullFace             = CullFace::BACK;

        gbufferPipelineDesc.colorAttachmentInfos[0].blendingEnabled = false;
        gbufferPipelineDesc.colorAttachmentInfos[1].blendingEnabled = false;
        gbufferPipelineDesc.colorAttachmentInfos[2].blendingEnabled = false;
        gbufferPipelineDesc.colorAttachmentInfos[3].blendingEnabled = false;
        gbufferPipelineDesc.colorAttachmentInfos[4].blendingEnabled = false;
        gbufferPipelineDesc.depthInfo.depthTestEnabled              = true;
        gbufferPipelineDesc.depthInfo.depthWriteEnabled             = true;
        gbufferPipelineDesc.depthInfo.compareFunc                   = CompareFunction::LESS;

        s_gbufferPipeline = Pipeline::Create( gbufferPipelineDesc );
        
        // lighting pass pipeline
        PipelineDescriptor directionalLightPipelineDesc;

        directionalLightPipelineDesc.numVertexDescriptors = 1;
        directionalLightPipelineDesc.vertexDescriptors    = &quadVertexDesc[0];
        directionalLightPipelineDesc.windingOrder         = WindingOrder::COUNTER_CLOCKWISE;
        directionalLightPipelineDesc.cullFace             = CullFace::BACK;

        auto& hdrColorAttachment = directionalLightPipelineDesc.colorAttachmentInfos[0];
        hdrColorAttachment.blendingEnabled     = true;
        hdrColorAttachment.colorBlendEquation  = BlendEquation::ADD;
        hdrColorAttachment.alphaBlendEquation  = BlendEquation::ADD;
        hdrColorAttachment.srcColorBlendFactor = BlendFactor::ONE;
        hdrColorAttachment.dstColorBlendFactor = BlendFactor::ONE;
        hdrColorAttachment.srcAlphaBlendFactor = BlendFactor::ONE;
        hdrColorAttachment.dstAlphaBlendFactor = BlendFactor::ONE;

        directionalLightPipelineDesc.depthInfo.depthTestEnabled  = false;
        directionalLightPipelineDesc.depthInfo.depthWriteEnabled = false;
        directionalLightPipelineDesc.depthInfo.compareFunc       = CompareFunction::LESS;

        s_directionalLightPipeline = Pipeline::Create( directionalLightPipelineDesc );

        // background pass pipeline
        PipelineDescriptor backgroundPipelineDesc;

        backgroundPipelineDesc.numVertexDescriptors = 1;
        backgroundPipelineDesc.vertexDescriptors    = &quadVertexDesc[0];
        backgroundPipelineDesc.windingOrder         = WindingOrder::COUNTER_CLOCKWISE;
        backgroundPipelineDesc.cullFace             = CullFace::BACK;

        backgroundPipelineDesc.colorAttachmentInfos[0].blendingEnabled = false;

        backgroundPipelineDesc.depthInfo.depthTestEnabled  = true;
        backgroundPipelineDesc.depthInfo.depthWriteEnabled = false;
        backgroundPipelineDesc.depthInfo.compareFunc       = CompareFunction::LESS;

        s_backgroundPipeline = Pipeline::Create( backgroundPipelineDesc );

        // postprocess pipeline
        PipelineDescriptor postProcessPipelineDesc;

        postProcessPipelineDesc.numVertexDescriptors = 1;
        postProcessPipelineDesc.vertexDescriptors    = &quadVertexDesc[0];
        postProcessPipelineDesc.windingOrder         = WindingOrder::COUNTER_CLOCKWISE;
        postProcessPipelineDesc.cullFace             = CullFace::BACK;

        postProcessPipelineDesc.colorAttachmentInfos[0].blendingEnabled = false;

        postProcessPipelineDesc.depthInfo.depthTestEnabled  = false;
        postProcessPipelineDesc.depthInfo.depthWriteEnabled = false;
        postProcessPipelineDesc.depthInfo.compareFunc       = CompareFunction::LESS;

        s_postProcessPipeline = Pipeline::Create( postProcessPipelineDesc );

        return true;
    }

    void Shutdown()
    {
        s_gbufferPipeline = {};
        s_directionalLightPipeline = {};
        s_backgroundPipeline = {};
        s_postProcessPipeline = {};

        s_quadVertexBuffer = {};
        s_cubeVertexBuffer = {};

        s_gbuffer.renderPass = {};
        s_gbuffer.positionTex = {};
        s_gbuffer.normalTex = {};
        s_gbuffer.diffuseTex = {};
        s_gbuffer.specularTex = {};
        s_gbuffer.emissiveTex = {};
        s_gbuffer.depthRbo = {};

        s_postProcessBuffer.renderPass = {};
        s_postProcessBuffer.hdrColorTex = {};

        s_screenRenderPass = {};

        for ( int i = 0; i < TOTAL_SHADERS; ++i )
        {
            s_shaders[i].Free();
        }

        for ( auto& [name, sampler] : s_samplers )
        {
            sampler = {};
        }
    }

    void Render( Scene* scene )
    {
        scene->SortLights();

        // ShadowPass(scene);

        GBufferPass( scene );

        LightingPass( scene );

        BackgroundPass( scene );

        PostProcessPass( scene );
    }

/*
    void ShadowPass(Scene* scene) {
        const auto& lights = scene->getLights();
        const auto& camera = *scene->getCamera();
        Frustum frustum = camera.GetFrustum();
        float np = camera.GetNearPlane();
        float fp = camera.GetFarPlane();
        glm::vec3 frustCenter = 0.5f*(fp - np) * camera.GetForwardDir() +
        camera.transform.position;

        auto numPointLights       = scene->getNumPointLights();
        auto numSpotLights        = scene->getNumSpotLights();
        auto numDirectionalLights = scene->getNumDirectionalLights();
        unsigned int index = 0;

        // point lights
        auto& pShader = shaders[ShaderNames::SHADOWPASS_POINT];
        pShader.enable();
        for (unsigned int i = 0; i < numPointLights; ++i) {
            Light* l = lights[index + i];
            ShadowMap* shadowMap = l->shadowMap;

            if (shadowMap) {
                const auto& pos = l->transform.position;
                glm::mat4 P = glm::perspective(glm::radians(90.0f), shadowMap->width() /
                (float)shadowMap->height(), 0.1f, l->radius); shadowMap->LSMs[0] = P * glm::lookAt(pos, pos +
                glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)); shadowMap->LSMs[1] = P *
                glm::lookAt(pos, pos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
                            shadowMap->LSMs[2] = P * glm::lookAt(pos, pos + glm::vec3(0.0f, 1.0f, 0.0f),
                glm::vec3(0.0f, 0.0f, 1.0f)); shadowMap->LSMs[3] = P * glm::lookAt(pos, pos + glm::vec3(0.0f,
                -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)); shadowMap->LSMs[4] = P * glm::lookAt(pos, pos +
                glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)); shadowMap->LSMs[5] = P *
                glm::lookAt(pos, pos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

                shadowMap->BindForWriting();
                pShader.setUniform("LSMs", shadowMap->LSMs, 6);
                pShader.setUniform("invFarPlane", 1.0f / l->radius);
                pShader.setUniform("lightPos", pos);
                depthRender(scene, pShader, glm::mat4(1));
            }
        }
        index += numPointLights;

        // spot lights
        auto& shader = s_shaders[ShaderNames::SHADOWPASS_DIRECTIONAL_SPOT];
        shader.enable();
        for (unsigned int i = 0; i < numSpotLights; ++i) {
            Light* l = lights[index + i];
            ShadowMap* shadowMap = l->shadowMap;

            if (shadowMap) {
                const auto& lightPos = l->transform.position;
                const auto lightDir = rotationToDirection(l->transform.rotation);
                const auto lightUp = rotationToDirection(l->transform.rotation, glm::vec3(0, 1,
                0));
                //const auto lightUp = glm::vec3(0, 1, 0);
                glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, lightUp);

                float aspect = shadowMap->width() / (float) shadowMap->height();
                glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), aspect, 0.1f,
    l->radius); shadowMap->LSMs[0] = lightProj * lightView;

                shadowMap->BindForWriting();
                depthRender(scene, shader, shadowMap->LSMs[0]);
            }
        }
        index += numSpotLights;

        // directional lights
        for (unsigned int i = 0; i < numDirectionalLights; ++i) {
            Light* l = lights[index + i];
            ShadowMap* shadowMap = l->shadowMap;

            if (shadowMap) {
                // create light space matrix (LSM)
                glm::vec3 lightDir = rotationToDirection(l->transform.rotation);
                glm::vec3 lightUp = rotationToDirection(l->transform.rotation, glm::vec3(0, 1,
                0)); glm::mat4 lightView = glm::lookAt(glm::vec3(0), lightDir, glm::vec3(0, 1,
                0));

                glm::vec3 LSCorners[8];
                for (int corner = 0; corner < 8; ++corner) {
                    LSCorners[corner] = glm::vec3(lightView * glm::vec4(frustum.corners[corner],
    1));
                }
                BoundingBox lsmBB(LSCorners[0], LSCorners[0]);
                lsmBB.Encompass(LSCorners + 1, 7);

                glm::vec3 pMin(-70, -70, -150);
                glm::vec3 pMax(70, 70, 150);
                glm::mat4 lightProj = glm::ortho<float>(pMin.x, pMax.x, pMin.y, pMax.y, pMin.z,
    pMax.z);
                // glm::mat4 lightProj = glm::ortho(lsmBB.min.x, lsmBB.max.x, lsmBB.min.y,
    lsmBB.max.y, lsmBB.min.z, lsmBB.max.z); shadowMap->LSMs[0] = lightProj * lightView;

                shadowMap->BindForWriting();
                depthRender(scene, shader, shadowMap->LSMs[0]);
            }
        }
    }
*/

    void GBufferPass( Scene* scene )
    {
        s_gbuffer.renderPass.Bind();

        auto& shader = s_shaders[ShaderNames::GBUFFER_PASS];
        shader.Enable();
        const auto& VP = scene->camera.GetVP();
        s_gbufferPipeline.Bind();

        ECS::component::for_each< ModelRenderComponent >(
            [&shader, &VP]( const ECS::Entity& e, ModelRenderComponent& c )
        {
            glm::mat4 M   = e->transform.getModelMatrix();
            glm::mat4 N   = glm::transpose( glm::inverse( M ) );
            glm::mat4 MVP = VP * M;
            shader.SetUniform( "M", M );
            shader.SetUniform( "N", N );
            shader.SetUniform( "MVP", MVP );
            for ( size_t i = 0; i < c.materials.size(); ++i )
            {
                const auto& mesh   = c.model->meshes[i];
                const auto& matPtr = c.materials[i];

                shader.SetUniform( "kd", matPtr->Kd );
                shader.SetUniform( "ks", matPtr->Ks );
                shader.SetUniform( "ke", matPtr->Ke );
                shader.SetUniform( "specular", matPtr->Ns );
                if ( matPtr->map_Kd )
                {
                    shader.SetUniform( "textured", true );
                    // matPtr->map_Kd->sampler->Bind( 0 );
                    s_samplers["nearest"].Bind( 0 );
                    shader.BindTexture( *matPtr->map_Kd->GetTexture(), "diffuseTex", 0 );
                }
                else
                {
                    shader.SetUniform( "textured", false );
                }

                Gfx::BindVertexBuffer( mesh.vertexBuffer, 0, mesh.GetVertexOffset(), 12 );
                Gfx::BindVertexBuffer( mesh.vertexBuffer, 1, mesh.GetNormalOffset(), 12 );
                Gfx::BindVertexBuffer( mesh.vertexBuffer, 2, mesh.GetUVOffset(), 8 );
                Gfx::BindIndexBuffer( mesh.indexBuffer );
                Gfx::DrawIndexedPrimitives( PrimitiveType::TRIANGLES, IndexType::UNSIGNED_INT, 0, mesh.GetNumIndices() );
            }
        } );
    }

    // TODO: Possible use tighter bounding light volumes, and look into using the stencil buffer
    //       to help not light pixels that intersect the volume only in screen space, not world
    void LightingPass( Scene* scene )
    {
        const auto& lights = scene->GetLights();
        const auto& VP     = scene->camera.GetVP();

        s_postProcessBuffer.renderPass.Bind();

        //Gfx::toggleDepthWrite( false );
        //Gfx::toggleDepthTest( true );

        auto numPointLights       = scene->GetNumPointLights();
        auto numSpotLights        = scene->GetNumSpotLights();
        auto numDirectionalLights = scene->GetNumDirectionalLights();
        unsigned int index        = 0;

        // // point lights
        // {
        //     auto& shader = s_shaders[ShaderNames::LIGHTPASS_POINT];
        //     shader.enable();
        //     s_gbuffer.BindForReading( shader );
        //     graphicsApi::bindVao( s_cubeVao );
        //     shader.setUniform( "cameraPos", scene->camera.position );
        //     for ( unsigned int i = 0; i < numPointLights; ++i )
        //     {
        //         Light* l = lights[index + i];
        //         shader.setUniform( "lightPos", l->position );
        //         shader.setUniform( "lightColor", l->color * l->intensity );
        //         shader.setUniform( "lightRadiusSquared", l->radius * l->radius );

        //         glm::mat4 M( 1 );
        //         M        = glm::translate( M, l->position );
        //         M        = glm::scale( M, glm::vec3( l->radius ) );
        //         auto MVP = VP * M;
        //         shader.setUniform( "MVP", MVP );

        //         // if (l->shadowMap) {
        //         //     graphicsApi::bindCubemap(l->shadowMap->texture(),
        //         //     shader.getUniform("shadowMap"), 5); shader.setUniform("shadows", true);
        //         //     shader.setUniform("shadowFarPlane", l->radius);
        //         // } else {
        //         //     shader.setUniform("shadows", false);
        //         // }

        //         float distToCam = glm::length( scene->camera.position - l->position );
        //         // if the camera is inside of the volume, then render the back faces instead
        //         if ( distToCam < sqrtf( 3.05f ) * l->radius )
        //         {
        //             graphicsApi::cullFace( GL_FRONT );
        //             graphicsApi::toggleDepthTest( false );
        //             glDrawArrays( GL_TRIANGLES, 0, 36 );
        //             graphicsApi::cullFace( GL_BACK );
        //             graphicsApi::toggleDepthTest( true );
        //         }
        //         else
        //         {
        //             glDrawArrays( GL_TRIANGLES, 0, 36 );
        //         }
        //     }
        //     index += numPointLights;
        // }

        // // spot lights
        // {
        //     auto& shader = s_shaders[ShaderNames::LIGHTPASS_SPOT];
        //     shader.enable();
        //     s_gbuffer.BindForReading( shader );
        //     graphicsApi::bindVao( s_cubeVao );
        //     shader.setUniform( "cameraPos", scene->camera.position );
        //     for ( unsigned int i = 0; i < numSpotLights; ++i )
        //     {
        //         Light* l = lights[index + i];
        //         shader.setUniform( "lightPos", l->position );
        //         shader.setUniform( "lightColor", l->color * l->intensity );
        //         shader.setUniform( "lightRadiusSquared", l->radius * l->radius );
        //         shader.setUniform( "lightInnerCutoff", glm::cos( l->innerCutoff ) );
        //         shader.setUniform( "lightOuterCutoff", glm::cos( l->outerCutoff ) );
        //         shader.setUniform( "lightDir", l->direction );

        //         // create a bounding box around the center (not beginning) of the spot light
        //         Transform t;
        //         t.position      = l->position + 0.5f * l->radius * l->direction;
        //         float maxExtent = glm::tan( l->outerCutoff ) * l->radius;
        //         t.scale         = glm::vec3( maxExtent, maxExtent, 0.5f * l->radius );
        //         auto MVP        = VP * t.getModelMatrix();
        //         shader.setUniform( "MVP", MVP );
        //         // if (l->shadowMap) {
        //         //     graphicsApi::bind2DTexture(l->shadowMap->texture(),
        //         //     shader.getUniform("shadowMap"), 5); shader.setUniform("LSM",
        //         //     l->shadowMap->LSMs[0]); shader.setUniform("shadows", true);
        //         // } else {
        //         //     shader.setUniform("shadows", false);
        //         // }

        //         maxExtent       = std::max( maxExtent, 0.5f * l->radius );
        //         float distToCam = glm::length( scene->camera.position - t.position );
        //         // if the camera is inside of the volume, then render the back faces instead
        //         if ( distToCam < sqrtf( 3.05 ) * maxExtent )
        //         {
        //             graphicsApi::cullFace( GL_FRONT );
        //             graphicsApi::toggleDepthTest( false );
        //             glDrawArrays( GL_TRIANGLES, 0, 36 );
        //             graphicsApi::cullFace( GL_BACK );
        //             graphicsApi::toggleDepthTest( true );
        //         }
        //         else
        //         {
        //             glDrawArrays( GL_TRIANGLES, 0, 36 );
        //         }
        //     }
        //     index += numSpotLights;
        // }

        // directional lights
        {
            auto& shader = s_shaders[ShaderNames::LIGHTPASS_DIRECTIONAL];
            shader.Enable();
            s_directionalLightPipeline.Bind();
            s_gbuffer.BindForReading( shader );
            shader.SetUniform( "cameraPos", scene->camera.position );
            for ( unsigned int i = 0; i < numDirectionalLights; ++i )
            {
                Light* l = lights[index + i];

                shader.SetUniform( "lightColor", l->color * l->intensity );
                shader.SetUniform( "lightDir", -l->direction );

                Gfx::BindVertexBuffer( s_quadVertexBuffer, 0, 0, 12 );
                Gfx::DrawNonIndexedPrimitives( PrimitiveType::TRIANGLES, 0, 6 );

                // if (l->shadowMap) {
                //     graphicsApi::bind2DTexture(l->shadowMap->texture(),
                //     shader.getUniform("shadowMap"), 5); shader.setUniform("LSM",
                //     l->shadowMap->LSMs[0]); shader.setUniform("shadows", true);
                // } else {
                //     shader.setUniform("shadows", false);
                // }
            }
        }
    }

    void BackgroundPass( Scene* scene )
    {
        PG_UNUSED( scene );
        Shader& shader = s_shaders[ShaderNames::BACKGROUND_OR_SKYBOX];
        shader.Enable();
        s_backgroundPipeline.Bind();
        //graphicsApi::toggleDepthWrite( false );

        // if (scene->skybox) {
        //     glm::mat4 P = scene->getCamera()->GetP();
        //     glm::mat4 RV = glm::mat4(glm::mat3(scene->getCamera()->GetV()));
        //     shader.setUniform("MVP", P * RV);
        //     shader.setUniform("skybox", true);
        //     graphicsApi::bindVao(cubeVao);
        //     graphicsApi::bindCubemap(scene->skybox->getGPUHandle(),
        // shader.getUniform("cubeMap"),
        //     0); glDrawArrays(GL_TRIANGLES, 0, 36);
        // } else {
            shader.SetUniform( "MVP", glm::mat4( 1 ) );
            shader.SetUniform( "skybox", false );
            shader.SetUniform( "color", scene->backgroundColor );
            Gfx::BindVertexBuffer( s_quadVertexBuffer, 0, 0, 12 );
            DrawNonIndexedPrimitives( PrimitiveType::TRIANGLES, 0, 6 );
        //}

        //graphicsApi::toggleDepthWrite( true );
    }

    void PostProcessPass( Scene* scene )
    {
        PG_UNUSED( scene );

        s_screenRenderPass.Bind();

        Blit( s_postProcessBuffer.renderPass, s_screenRenderPass, s_window->Width(), s_window->Height(),
              RenderTargetBuffers::RENDER_TARGET_DEPTH, FilterMode::NEAREST );

        auto& shader = s_shaders[ShaderNames::POST_PROCESS];
        shader.Enable();
        s_postProcessPipeline.Bind();

        // shader.setUniform("exposure", 1.0f); // TODO: Actually calculate exposure
        shader.BindTexture( s_postProcessBuffer.hdrColorTex, "originalColor", 0 );
        Gfx::BindVertexBuffer( s_quadVertexBuffer, 0, 0, 12 );
        Gfx::DrawNonIndexedPrimitives( PrimitiveType::TRIANGLES, 0, 6 );

        /*
        // Bind and clear the main screen
        // graphicsApi::bindFramebuffer( 0 );
        // graphicsApi::setViewport( s_window->Width(), s_window->Height() );
        // graphicsApi::clearColor( 0, 0, 0, 0 );
        // graphicsApi::clearColorBuffer();

        // // Copy the post processing depth buffer to the main screen
        // // graphicsApi::blitFboToFbo(postProcessBuffer.fbo, 0, Window::width(), Window::height(),
        // // GL_DEPTH_BUFFER_BIT);

        // graphicsApi::toggleDepthWrite( false );
        // graphicsApi::toggleDepthTest( false );

        // // Draw the post processing color texture to screen, while performing
        // // post processing effects, tone mapping, and gamma correction
        // auto& shader = s_shaders[ShaderNames::POST_PROCESS];
        // shader.enable();
        // // shader.setUniform("exposure", 1.0f); // TODO: Actually calculate exposure
        // graphicsApi::bind2DTexture( s_postProcessBuffer.hdrColorTex,
        //                             shader.getUniform( "originalColor" ), 0 );
        // graphicsApi::bindVao( s_quadVao );
        // glDrawArrays( GL_TRIANGLES, 0, 6 );

        // graphicsApi::toggleDepthWrite( true );
        // graphicsApi::toggleDepthTest( true );
        */
    }

    void InitSamplers()
    {
        SamplerDescriptor samplerDesc;

        samplerDesc.minFilter = FilterMode::NEAREST;
        samplerDesc.magFilter = FilterMode::NEAREST;
        samplerDesc.wrapModeS = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeT = WrapMode::CLAMP_TO_EDGE;
        samplerDesc.wrapModeR = WrapMode::CLAMP_TO_EDGE;
        s_samplers["nearest"] = Sampler::Create( samplerDesc );

        samplerDesc.minFilter = FilterMode::LINEAR;
        samplerDesc.magFilter = FilterMode::LINEAR;
        s_samplers["linear"] = Sampler::Create( samplerDesc );
    }


} // namespace RenderSystem
} // namespace Progression
