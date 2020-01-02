#include "progression.hpp"
#include <thread>

using namespace Progression;

extern int g_debugLayer;
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
        // window->SetRelativeMouse( true );
    
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

            if ( PG::Input::GetKeyUp( PG::Key::K_0 ) )
            {
                g_debugLayer = 0;
            }
            if ( PG::Input::GetKeyUp( PG::Key::K_1 ) )
            {
                g_debugLayer = 1;
            }
            if ( PG::Input::GetKeyUp( PG::Key::K_2 ) )
            {
                g_debugLayer = 2;
            }
            if ( PG::Input::GetKeyUp( PG::Key::K_3 ) )
            {
                g_debugLayer = 3;
            }
            if ( PG::Input::GetKeyUp( PG::Key::K_4 ) )
            {
                g_debugLayer = 4;
            }
            if ( PG::Input::GetKeyUp( PG::Key::K_5 ) )
            {
                g_debugLayer = 5;
            }
            if ( PG::Input::GetKeyUp( PG::Key::K_6 ) )
            {
                g_debugLayer = 6;
            }
            if ( PG::Input::GetKeyUp( PG::Key::K_7 ) )
            {
                g_debugLayer = 7;
            }
            if ( PG::Input::GetKeyUp( PG::Key::K_8 ) )
            {
                g_debugLayer = 8;
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
