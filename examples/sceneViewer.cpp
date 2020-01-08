#include "progression.hpp"
#include <thread>

using namespace Progression;

bool g_paused = false;

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
            if ( Input::GetKeyUp( Key::F1 ) )
            {
                Gfx::UIOverlay::SetVisible( !Gfx::UIOverlay::Visible() );
            }
            if ( PG::Input::GetKeyUp( PG::Key::P ) )
            {
                g_paused = !g_paused;
            }


            if ( !g_paused )
            {
                scene->Update();
            }
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
