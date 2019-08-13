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
#include "resource/texture.hpp"
//#include "graphics/shadow_map.hpp"
#include <array>

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
template < class T >
inline void hash_combine( std::size_t& seed, const T& v )
{
    std::hash< T > hasher;
    seed ^= hasher( v ) + 0x9e3779b9 + ( seed << 6 ) + ( seed >> 2 );
}

namespace std
{
template <>
struct hash< Gfx::SamplerDescriptor >
{
    std::size_t operator()( const Gfx::SamplerDescriptor& s ) const noexcept
    {
        std::size_t h( std::hash< int >{}( (int) s.minFilter ) );
        hash_combine( h, (int) s.minFilter );
        hash_combine( h, (int) s.magFilter );
        hash_combine( h, (int) s.wrapModeS );
        hash_combine( h, (int) s.wrapModeT );
        hash_combine( h, (int) s.wrapModeR );
        hash_combine( h, s.borderColor.x );
        hash_combine( h, s.borderColor.y );
        hash_combine( h, s.borderColor.z );
        hash_combine( h, s.borderColor.w );
        hash_combine( h, s.maxAnisotropy );

        return h;
    }
};
} // namespace std

static std::unordered_map< std::size_t, Gfx::Sampler > s_samplers;

namespace Progression
{
namespace RenderSystem
{

    Gfx::Sampler* GetSampler( Gfx::SamplerDescriptor* desc )
    {
        auto hash = std::hash< Gfx::SamplerDescriptor >{}( *desc );
        auto it   = s_samplers.find( hash );
        if ( it != s_samplers.end() )
        {
            return &it->second;
        }
        s_samplers[hash] = Gfx::Sampler::Create( *desc );
        return &s_samplers[hash];
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

    void BindForWriting()
    {
        // graphicsApi::bindFramebuffer( fbo );
        // graphicsApi::clearColor( 0, 0, 0, 0 );
        // graphicsApi::clearColorBuffer();
        // graphicsApi::clearDepthBuffer();
    }

    void BindForReading( Shader& shader )
    {
        PG_UNUSED( shader );
        // graphicsApi::bind2DTexture( positionTex, shader.getUniform( "gPosition" ), 0 );
        // graphicsApi::bind2DTexture( normalTex, shader.getUniform( "gNormal" ), 1 );
        // graphicsApi::bind2DTexture( diffuseTex, shader.getUniform( "gDiffuse" ), 2 );
        // graphicsApi::bind2DTexture( specularTex, shader.getUniform( "gSpecularExp" ), 3 );
        // graphicsApi::bind2DTexture( emissiveTex, shader.getUniform( "gEmissive" ), 4 );

    }
};

struct PostProcessBuffer
{
    RenderPass renderPass;
    Gfx::Texture hdrColorTex;
    Gfx::Texture depthRbo;
};

static RenderPass s_screenFrameBuffer;
static GBuffer s_gbuffer;
static PostProcessBuffer s_postProcessBuffer;
static VertexInputDescriptor s_meshVao;
static VertexInputDescriptor s_quadVao;
static Buffer s_quadVertexBuffer;
static VertexInputDescriptor s_cubeVao;
static Buffer s_cubeVertexBuffer;
static Window* s_window;
static Viewport s_windowViewport;

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

    // void shadowPass(Scene* scene);
    void GBufferPass( Scene* scene );
    void LightingPass( Scene* scene );
    void BackgroundPass( Scene* scene );
    void PostProcessPass( Scene* scene );

    bool Init()
    {
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

        s_meshVao = VertexInputDescriptor::Create( 3, &meshAttribDescs[0] );

        // load quad data
        float quadVerts[] =
        {
            -1, 1,  -1, -1,   1, -1,
            -1, 1,   1, -1,   1, 1
        };
        
        std::array< VertexAttributeDescriptor, 1 > attribDescs;
        attribDescs[0].binding  = 0;
        attribDescs[0].location = 0;
        attribDescs[0].count    = 2;
        attribDescs[0].format   = BufferDataType::FLOAT32;
        attribDescs[0].offset   = 0;

        s_quadVao = VertexInputDescriptor::Create( 1, &attribDescs[0] );
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

        attribDescs[0].binding  = 0;
        attribDescs[0].location = 0;
        attribDescs[0].count    = 3;
        attribDescs[0].format   = BufferDataType::FLOAT32;
        attribDescs[0].offset   = 0;

        s_cubeVao = VertexInputDescriptor::Create( 1, &attribDescs[0] );
        s_cubeVertexBuffer = Buffer::Create( skyboxVertices, sizeof( skyboxVertices ), BufferType::VERTEX, BufferUsage::STATIC );

        s_window = GetMainWindow();

        TextureDescriptor texDesc;
        texDesc.width     = s_window->Width();
        texDesc.height    = s_window->Height();
        texDesc.type      = TextureType::TEXTURE2D;
        texDesc.mipmapped = false;
        texDesc.format    = PixelFormat::R32_G32_B32_A32_Float;
        
        s_gbuffer.positionTex = Gfx::Texture::Create( texDesc, nullptr, texDesc.format );
        s_gbuffer.normalTex   = Gfx::Texture::Create( texDesc, nullptr, texDesc.format );

        texDesc.format = PixelFormat::R16_G16_B16_A16_Float;
        s_gbuffer.specularTex = Gfx::Texture::Create( texDesc, nullptr, texDesc.format );

        texDesc.format = PixelFormat::R16_G16_B16_Float;
        s_gbuffer.emissiveTex = Gfx::Texture::Create( texDesc, nullptr, texDesc.format );

        texDesc.format = PixelFormat::R8_G8_B8_Uint; // TODO: should this be sRGB?
        s_gbuffer.diffuseTex  = Gfx::Texture::Create( texDesc, nullptr, texDesc.format );

        texDesc.format = PixelFormat::DEPTH32_Float;
        s_gbuffer.depthRbo  = Gfx::Texture::Create( texDesc, nullptr, texDesc.format );

        std::vector< ColorAttachmentDescriptor > colorAttachments( 5 );
        colorAttachments[0].texture = &s_gbuffer.positionTex;
        colorAttachments[1].texture = &s_gbuffer.normalTex;
        colorAttachments[2].texture = &s_gbuffer.specularTex;
        colorAttachments[3].texture = &s_gbuffer.emissiveTex;
        colorAttachments[4].texture = &s_gbuffer.diffuseTex;

        DepthAttachmentDescriptor depthAttachment;
        depthAttachment.texture = &s_gbuffer.depthRbo;

        RenderPassDescriptor gBufferDescriptor;
        gBufferDescriptor.SetColorAttachments( colorAttachments );
        gBufferDescriptor.SetDepthAttachment( depthAttachment );
        s_gbuffer.renderPass = RenderPass::Create( gBufferDescriptor );

        texDesc.format = PixelFormat::R16_G16_B16_A16_Float;
        s_postProcessBuffer.depthRbo  = Gfx::Texture::Create( texDesc, nullptr, texDesc.format );
        texDesc.format = PixelFormat::DEPTH32_Float;
        s_postProcessBuffer.depthRbo  = Gfx::Texture::Create( texDesc, nullptr, texDesc.format );
        
        colorAttachments.resize( 1 );
        colorAttachments[0].texture = &s_postProcessBuffer.hdrColorTex;
        depthAttachment.texture     = &s_postProcessBuffer.depthRbo;

        RenderPassDescriptor postProcessDescriptor;
        postProcessDescriptor.SetColorAttachments( colorAttachments );
        postProcessDescriptor.SetDepthAttachment( depthAttachment );
        s_postProcessBuffer.renderPass = RenderPass::Create( postProcessDescriptor );

        s_screenFrameBuffer = RenderPass::CreateDefault( postProcessDescriptor );

        s_windowViewport.x = 0;
        s_windowViewport.y = 0;
        s_windowViewport.width = s_window->Width();
        s_windowViewport.height = s_window->Height();
        SetViewport( s_windowViewport );

        EnableBlending( false );
        SetWindingOrder( WindingOrder::COUNTER_CLOCKWISE );
        SetCullFace( CullFace::BACK );

        return true;
    }

    void Shutdown()
    {
        s_quadVertexBuffer = {};
        s_quadVao = {};
        s_cubeVao = {};
        s_cubeVertexBuffer = {};
        s_meshVao = {};

        s_gbuffer.renderPass = {};
        s_gbuffer.positionTex = {};
        s_gbuffer.normalTex = {};
        s_gbuffer.diffuseTex = {};
        s_gbuffer.specularTex = {};
        s_gbuffer.emissiveTex = {};
        s_gbuffer.depthRbo = {};

        s_postProcessBuffer.renderPass = {};
        s_postProcessBuffer.hdrColorTex = {};
        s_postProcessBuffer.depthRbo = {};

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

        // shadowPass(scene);

        GBufferPass( scene );

        // write the result of the lighting pass to the post processing FBO
        // Gfx::bindFramebuffer( s_postProcessBuffer.fbo );
        // Gfx::clearColor( 0, 0, 0, 0 );
        // Gfx::clearColorBuffer();
        // Gfx::blitFboToFbo( s_gbuffer.fbo, s_postProcessBuffer.fbo, s_window->Width(),
        //  s_window->Height(), GL_DEPTH_BUFFER_BIT );

        LightingPass( scene );

        BackgroundPass( scene );

        PostProcessPass( scene );
    }

/*
    void shadowPass(Scene* scene) {
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
        PG_UNUSED( scene );
        s_gbuffer.renderPass.Bind();
        
        //Gfx::SetViewport( s_windowViewport );
        //Gfx::SetCullFace( CullFace::BACK );

        auto& shader = s_shaders[ShaderNames::GBUFFER_PASS];
        shader.Enable();
        const auto& VP = scene->camera.GetVP();

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
                s_meshVao.Bind();
                shader.SetUniform( "kd", matPtr->Kd );
                shader.SetUniform( "ks", matPtr->Ks );
                shader.SetUniform( "ke", matPtr->Ke );
                shader.SetUniform( "specular", matPtr->Ns );
                if ( matPtr->map_Kd )
                {
                    shader.SetUniform( "textured", true );
                    matPtr->map_Kd->sampler->Bind( 0 );
                    shader.BindTexture( matPtr->map_Kd->gfxTexture, "diffuseTex", 0 );
                }
                else
                {
                    shader.SetUniform( "textured", false );
                };
                glDrawElements( GL_TRIANGLES, mesh.GetNumIndices(), GL_UNSIGNED_INT, 0 );
            }
        } );
    }

    // TODO: Possible use tighter bounding light volumes, and look into using the stencil buffer
    //       to help not light pixels that intersect the volume only in screen space, not world
    void LightingPass( Scene* scene )
    {
        const auto& lights = scene->GetLights();
        const auto& VP     = scene->camera.GetVP();

        Gfx::EnableBlending( true );
        Gfx::SetBlendEquations( Gfx::BlendEquation::ADD, Gfx::BlendEquation::ADD );
        Gfx::SetBlendFactors( Gfx::BlendFactor::ONE, Gfx::BlendFactor::ONE, Gfx::BlendFactor::ONE, Gfx::BlendFactor::ONE );

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

        // graphicsApi::toggleCulling( false );
        // graphicsApi::toggleDepthTest( false );
        // // directional lights
        // {
        //     auto& shader = s_shaders[ShaderNames::LIGHTPASS_DIRECTIONAL];
        //     shader.enable();
        //     s_gbuffer.BindForReading( shader );
        //     graphicsApi::bindVao( s_quadVao );
        //     shader.setUniform( "cameraPos", scene->camera.position );
        //     for ( unsigned int i = 0; i < numDirectionalLights; ++i )
        //     {
        //         Light* l = lights[index + i];

        //         shader.setUniform( "lightColor", l->color * l->intensity );
        //         shader.setUniform( "lightDir", -l->direction );

        //         // if (l->shadowMap) {
        //         //     graphicsApi::bind2DTexture(l->shadowMap->texture(),
        //         //     shader.getUniform("shadowMap"), 5); shader.setUniform("LSM",
        //         //     l->shadowMap->LSMs[0]); shader.setUniform("shadows", true);
        //         // } else {
        //         //     shader.setUniform("shadows", false);
        //         // }

        //         glDrawArrays( GL_TRIANGLES, 0, 6 );
        //     }
        // }

        // graphicsApi::toggleBlending( false );
        // graphicsApi::toggleDepthWrite( true );
        // graphicsApi::toggleDepthTest( true );
    }

    void BackgroundPass( Scene* scene )
    {
        Shader& shader = s_shaders[ShaderNames::BACKGROUND_OR_SKYBOX];
        shader.Enable();
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
        s_quadVao.Bind();
        DrawNonIndexedPrimitives( PrimitiveType::TRIANGLES, 0, 6 );
        //glDrawArrays( GL_TRIANGLES, 0, 6 );
        //}

        //graphicsApi::toggleDepthWrite( true );
    }

    void PostProcessPass( Scene* scene )
    {
        PG_UNUSED( scene );
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
    }


} // namespace RenderSystem
} // namespace Progression
