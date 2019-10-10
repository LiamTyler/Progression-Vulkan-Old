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

    if ( !PG::EngineInitialize() )
    {
        std::cout << "Failed to initialize the engine" << std::endl;
        return 1;
    }

    {
        Window* window = GetMainWindow();
        // window->setRelativeMouse(true);
        
        Scene* scene = Scene::Load( PG_RESOURCE_DIR "scenes/scene1.txt" );

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

            window->EndFrame();
        }
    }

    PG::EngineQuit();

    return 0;
}
