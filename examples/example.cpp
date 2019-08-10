#include "progression.hpp"
#include <future>
#include <iomanip>
#include <thread>

#ifdef __linux__
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

using namespace Progression;
using namespace Progression::Gfx;

int main( int argc, char* argv[] )
{
    PG_UNUSED( argc );
    PG_UNUSED( argv );

    PG::EngineInitialize();

    Window* window = GetMainWindow();
    // window->setRelativeMouse(true);

    // Scene* scene = Scene::load( PG_RESOURCE_DIR "scenes/scene1.txt" );
    ResourceManager::LoadFastFile( PG_RESOURCE_DIR "fastfiles/resource.txt.ff" );


    PG_ASSERT( ResourceManager::Get< Shader >( "test" ) );
    Shader& shader = *ResourceManager::Get< Shader >( "test" );

    PG_ASSERT( ResourceManager::Get<::Progression::Texture >( "cockatoo" ) );
    auto tex = ResourceManager::Get<::Progression::Texture >( "cockatoo" );

    PG_ASSERT( ResourceManager::Get< Material >( "cockatooMaterial" ) );
    auto mat = ResourceManager::Get< Material >( "cockatooMaterial" );

    PG_ASSERT( ResourceManager::Get< Model >( "chalet2" ) );
    Model& model = *ResourceManager::Get< Model >( "chalet2" );

    auto mat2 = std::make_shared< Material>();
    mat2->Ka     = glm::vec3( 0 );
    mat2->Kd     = glm::vec3( 1, 1, 1 );
    mat2->Ks     = glm::vec3( 0 );
    mat2->Ke     = glm::vec3( 0 );
    mat2->Ns     = 50;
    mat2->map_Kd = model.materials[0]->map_Kd;

    // float quadVerts[] = {
    //     -1, 1, 0, 0, 0, 1, 0, 1, -1, -1, 0, 0, 0, 1, 0, 0, 1, -1, 0, 0, 0, 1, 1, 0,

    //     -1, 1, 0, 0, 0, 1, 0, 1, 1,  -1, 0, 0, 0, 1, 1, 0, 1, 1,  0, 0, 0, 1, 1, 1,
    // }; 
    // uint16_t indices[] = { 0, 1, 2, 3, 4, 5 };

    Mesh meshTmp;
    meshTmp.vertices.push_back( { -1, 1, 0 } );
    meshTmp.vertices.push_back( { -1, -1, 0 } );
    meshTmp.vertices.push_back( { 1, -1, 0 } );
    meshTmp.vertices.push_back( { -1, 1, 0 } );
    meshTmp.vertices.push_back( { 1, -1, 0 } );
    meshTmp.vertices.push_back( { 1, 1, 0 } );
    meshTmp.normals.push_back( { 0, 0, 1 } );
    meshTmp.normals.push_back( { 0, 0, 1 } );
    meshTmp.normals.push_back( { 0, 0, 1 } );
    meshTmp.normals.push_back( { 0, 0, 1 } );
    meshTmp.normals.push_back( { 0, 0, 1 } );
    meshTmp.normals.push_back( { 0, 0, 1 } );
    meshTmp.uvs.push_back( { 0, 1 } );
    meshTmp.uvs.push_back( { 0, 0 } );
    meshTmp.uvs.push_back( { 1, 0 } );
    meshTmp.uvs.push_back( { 0, 1 } );
    meshTmp.uvs.push_back( { 1, 0 } );
    meshTmp.uvs.push_back( { 1, 1 } );
    meshTmp.indices = { 0, 1, 2, 3, 4, 5 };
    meshTmp.UploadToGpu();

    Model model2;
    model2.meshes.push_back( std::move( meshTmp ) );
    model2.materials.push_back( mat2 );

    DepthAttachmentDescriptor depthAttachmentDesc;
    depthAttachmentDesc.depthTestEnabled = true;
    depthAttachmentDesc.compareFunc = Gfx::CompareFunction::LESS;

    ColorAttachmentDescriptor colorAttachmentDescriptor;
    colorAttachmentDescriptor.clearColor = glm::vec4( 0, 0, 0, 0 );

    RenderPassDescriptor renderPassDesc;
    renderPassDesc.SetDepthAttachment( depthAttachmentDesc );
    renderPassDesc.SetColorAttachments( { colorAttachmentDescriptor } );

    RenderPass renderPass = RenderPass::CreateDefault( renderPassDesc );

    {
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

        VertexInputDescriptor vertexInputDesc = VertexInputDescriptor::Create( 3, &attribDescs[0] );


        PG::Input::PollEvents();

        Transform modelTransform( glm::vec3( 0, 0, 0 ), glm::vec3( glm::radians( -90.0f ), glm::radians( 90.0f ), 0 ), glm::vec3( 1 ) );
        Camera camera( glm::vec3( 0, 0, 3 ), glm::vec3( 0 ) );

        glm::vec3 lightDir = -glm::normalize( glm::vec3( 0, 0, -1 ) );

        PG::Input::PollEvents();

        // Game loop
        while ( !PG::EngineShutdown )
        {
            window->StartFrame();
            PG::Input::PollEvents();

            if ( PG::Input::GetKeyDown( PG::PG_K_ESC ) )
                PG::EngineShutdown = true;

            renderPass.Bind();

            shader.Enable();
            glm::mat4 M   = modelTransform.getModelMatrix();
            glm::mat4 N   = glm::transpose( glm::inverse( M ) );
            glm::mat4 MVP = camera.GetVP() * M;
            shader.SetUniform( "M", M );
            shader.SetUniform( "N", N );
            shader.SetUniform( "MVP", MVP );

            shader.SetUniform( "cameraPos", camera.position );
            shader.SetUniform( "lightDirInWorldSpace", lightDir );


            for ( size_t i = 0; i < model.meshes.size(); ++i )
            {
                Mesh& mesh    = model.meshes[i];
                auto& matPtr  = model.materials[i];

                vertexInputDesc.Bind();
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


            window->EndFrame();
        }
    }

    PG::EngineQuit();

    return 0;
}
