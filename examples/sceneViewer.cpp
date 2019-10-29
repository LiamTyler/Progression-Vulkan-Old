#include "progression.hpp"
#include <thread>

using namespace Progression;

int main( int argc, char* argv[] )
{
    if ( argc != 2 )
    {
        std::cout << "Usage: ./example [path to scene file]" << std::endl;
        return 0;
    }

    if ( !PG::EngineInitialize() )
    {
        std::cout << "Failed to initialize the engine" << std::endl;
        return 0;
    }

    Window* window = GetMainWindow();
    // window->setRelativeMouse(true);
    
    Scene* scene = Scene::Load( argv[1] );
    if ( !scene )
    {
        LOG_ERR( "Could not load scene '", argv[1], "'" );
        PG::EngineQuit();
        return 0;
    }

    std::string fbxFile = PG_RESOURCE_DIR "dragon/Dragon_Baked_Actions_fbx_7.4_binary.fbx";

    std::vector< std::shared_ptr< SkinnedModel > > skinnedModels;
    if ( !SkinnedModel::LoadFBX( fbxFile, skinnedModels ) )
    {
        LOG_ERR( "Could not load the fbx file '", fbxFile, "'" );
        PG::EngineQuit();
        return 0;
    }

    for ( const auto& m : skinnedModels )
    {
        auto entity             = scene->registry.create();
        auto& transform         = scene->registry.assign< Transform >( entity );
        // transform.position      = -0.25f * ( pgModel->aabb.min + pgModel->aabb.max );
        transform.position      = glm::vec3( 0, -20, -70 );
        LOG( "transform position = ", transform.position );
        transform.rotation      = glm::vec3( glm::radians( -50.0f ), glm::radians( 20.0f ), 0 );
        transform.scale         = glm::vec3( 1 );
        auto& skinned_renderer  = scene->registry.assign< SkinnedRenderer >( entity );
        skinned_renderer.model  = m;
    }

    //LOG( "Model AABB min = ", pgModel->aabb.min );
    //LOG( "Model AABB max = ", pgModel->aabb.max );
    //auto entity             = scene->registry.create();
    //auto& transform         = scene->registry.assign< Transform >( entity );
    //// transform.position      = -0.25f * ( pgModel->aabb.min + pgModel->aabb.max );
    //transform.position      = glm::vec3( 0, -20, -70 );
    //LOG( "transform position = ", transform.position );
    //transform.rotation      = glm::vec3( glm::radians( -50.0f ), glm::radians( 20.0f ), 0 );
    //transform.scale         = glm::vec3( 1 );
    //auto& modelRenderer     = scene->registry.assign< ModelRenderer >( entity );
    //modelRenderer.model     = pgModel;
    //modelRenderer.materials = pgModel->materials;
    //ResourceManager::Add< Model >( pgModel );

    scene->Start();

    PG::Input::PollEvents();

    // Game loop
    while ( !PG::g_engineShutdown )
    {
        window->StartFrame();
        PG::Input::PollEvents();

        if ( PG::Input::GetKeyDown( PG::Key::ESC ) )
        {
            PG::g_engineShutdown = true;
        }

        // scene->Update();
        RenderSystem::Render( scene );

        // std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

        window->EndFrame();
    }

    delete scene;

    PG::EngineQuit();

    return 0;
}
