#include "progression.hpp"
#include <future>
#include <iomanip>
#include <thread>
#include <array>

#ifdef __linux__
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif

using namespace Progression;
using namespace Progression::Gfx;

int main( int argc, char* argv[] )
{
    PG_UNUSED( argc );
    PG_UNUSED( argv );

    PG::EngineInitialize();

    {
        Window* window = GetMainWindow();
        // window->setRelativeMouse(true);

        Scene* scene = Scene::Load( PG_RESOURCE_DIR "scenes/scene1.txt" );
        // ResourceManager::LoadFastFile( PG_RESOURCE_DIR "fastfiles/resource.txt.ff" );

        PG_ASSERT( ResourceManager::Get< Shader >( "test" ) );
        Shader& shader = *ResourceManager::Get< Shader >( "test" );

        PG_ASSERT( ResourceManager::Get<::Progression::Texture >( "cockatoo" ) );
        auto tex = ResourceManager::Get<::Progression::Texture >( "cockatoo" );

        PG_ASSERT( ResourceManager::Get< Material >( "cockatooMaterial" ) );
        auto mat = ResourceManager::Get< Material >( "cockatooMaterial" );

        PG_ASSERT( ResourceManager::Get< Model >( "chalet2" ) );
        Model& model = *ResourceManager::Get< Model >( "chalet2" );
        Model& drawModel = model;

        RenderPassDescriptor renderPassDesc;
        renderPassDesc.colorAttachmentDescriptors[0].clearColor = glm::vec4( 0, 0, 0, 1 );
        RenderPass renderPass = RenderPass::Create( renderPassDesc );

        PipelineDescriptor pipelineDesc;

        std::array< VertexAttributeDescriptor, 3 > attribDescs;
        attribDescs[0].binding  = 0;
        attribDescs[0].location = 0;
        attribDescs[0].count    = 3;
        attribDescs[0].format   = BufferDataType::FLOAT32;
        attribDescs[0].offset   = 0;

        attribDescs[1].binding  = 1;
        attribDescs[1].location = 1;
        attribDescs[1].count    = 3;
        attribDescs[1].format   = BufferDataType::FLOAT32;
        attribDescs[1].offset   = 0;

        attribDescs[2].binding  = 2;
        attribDescs[2].location = 2;
        attribDescs[2].count    = 2;
        attribDescs[2].format   = BufferDataType::FLOAT32;
        attribDescs[2].offset   = 0;

        pipelineDesc.numVertexDescriptors   = 3;
        pipelineDesc.vertexDescriptors      = &attribDescs[0];
        pipelineDesc.windingOrder           = WindingOrder::COUNTER_CLOCKWISE;
        pipelineDesc.cullFace               = CullFace::BACK;

        pipelineDesc.colorAttachmentInfos[0].blendingEnabled = false;
        pipelineDesc.depthInfo.depthTestEnabled     = true;
        pipelineDesc.depthInfo.depthWriteEnabled    = true;
        pipelineDesc.depthInfo.compareFunc          = CompareFunction::LESS;

        Pipeline pipeline = Pipeline::Create( pipelineDesc );

        PG::Input::PollEvents();

        // Transform modelTransform( glm::vec3( 0, 0, 0 ), glm::vec3( glm::radians( -0.0f ), glm::radians( 0.0f ), 0 ), glm::vec3( 1 ) );
        Transform modelTransform( glm::vec3( 0, -0.5, 0 ),
                                  glm::vec3( glm::radians( -90.0f ), glm::radians( 90.0f ), 0 ),
                                  glm::vec3( 1 ) );
        Camera camera( glm::vec3( 0, 0, 3 ), glm::vec3( 0 ) );

        glm::vec3 lightDir = -glm::normalize( glm::vec3( 0, 0, -1 ) );

        PG::Input::PollEvents();

        // Game loop
        while ( !PG::EngineShutdown )
        {
            window->StartFrame();
            PG::Input::PollEvents();

            if ( PG::Input::GetKeyDown( PG::PG_K_ESC ) )
            {
                PG::EngineShutdown = true;
            }

            RenderSystem::Render( scene );

            /*
            renderPass.Bind();
            shader.Enable();
            pipeline.Bind();

            glm::mat4 M   = modelTransform.getModelMatrix();
            glm::mat4 N   = glm::transpose( glm::inverse( M ) );
            glm::mat4 MVP = camera.GetVP() * M;
            shader.SetUniform( "M", M );
            shader.SetUniform( "N", N );
            shader.SetUniform( "MVP", MVP );

            shader.SetUniform( "cameraPos", camera.position );
            shader.SetUniform( "lightDirInWorldSpace", lightDir );

            for ( size_t i = 0; i < drawModel.meshes.size(); ++i )
            {
                Mesh& mesh    = drawModel.meshes[i];
                auto& matPtr  = drawModel.materials[i];

                BindVertexBuffer( mesh.vertexBuffer, 0, mesh.GetVertexOffset(), 12 );
                BindVertexBuffer( mesh.vertexBuffer, 1, mesh.GetNormalOffset(), 12 );
                BindVertexBuffer( mesh.vertexBuffer, 2, mesh.GetUVOffset(), 8 );
                BindIndexBuffer( mesh.indexBuffer );

                shader.SetUniform( "kd", matPtr->Kd );
                shader.SetUniform( "ks", matPtr->Ks );
                shader.SetUniform( "ke", matPtr->Ke );
                shader.SetUniform( "shininess", matPtr->Ns );
                if ( matPtr->map_Kd )
                {
                    shader.SetUniform( "textured", true );
                    matPtr->map_Kd->sampler->Bind( 0 );
                    shader.BindTexture( matPtr->map_Kd->gfxTexture, "diffuseTex", 0 );
                }
                else
                {
                    shader.SetUniform( "textured", false );
                }

                DrawIndexedPrimitives( PrimitiveType::TRIANGLES, IndexType::UNSIGNED_INT, 0, mesh.GetNumIndices() );
            }
            */

            window->EndFrame();
        }
    }

    PG::EngineQuit();

    return 0;
}
