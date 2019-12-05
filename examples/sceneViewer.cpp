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

    {
        Window* window = GetMainWindow();
        window->SetRelativeMouse( true );
    
        Scene* scene = Scene::Load( argv[1] );
        if ( !scene )
        {
            LOG_ERR( "Could not load scene '", argv[1], "'" );
            PG::EngineQuit();
            return 0;
        }
    
        /*
        std::shared_ptr< Model > model = std::make_shared< Model >();
        ModelCreateInfo info;
        info.name = "sponza";
        info.filename = PG_RESOURCE_DIR "Sponza-master/sponza.obj";
        info.optimize = true;
        if ( !model->Load( &info ) )
        {
            LOG_ERR( "Could not load the fbx file '", info.filename, "'" );
            PG::EngineQuit();
            return 0;
        }

        {
            auto entity        = scene->registry.create();
            auto& transform    = scene->registry.assign< Transform >( entity );
            transform.position = glm::vec3( 0, 0, 0 );
            // transform.rotation = glm::vec3( glm::radians( -90.0f ), glm::radians( 90.0f ), 0 );
            transform.scale    = glm::vec3( .01f );
            auto& renderer     = scene->registry.assign< ModelRenderer >( entity );
            renderer.model     = model;
            renderer.materials = model->materials;
        }
        */

        scene->Start();

        PG::Input::PollEvents();
        Time::Reset();

        // Game loop
        while ( !PG::g_engineShutdown )
        {
            window->StartFrame();
            PG::Input::PollEvents();

            if ( PG::Input::GetKeyDown( PG::Key::ESC ) )
            {
                PG::g_engineShutdown = true;
            }

            scene->Update();
            RenderSystem::Render( scene );

            // std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

            window->EndFrame();
        }

        PG::Gfx::g_renderState.device.WaitForIdle();
        delete scene;
    }

    PG::EngineQuit();

    return 0;
}
