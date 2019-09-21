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

    Shader shader;
    ShaderCreateInfo info;
    info.filename = PG_RESOURCE_DIR "shaders/simple.frag.spv";
    if ( !shader.Load( &info ) )
    {
        LOG_ERR( "Could not load shader" );
        PG::EngineQuit();
        return 1;
    }

    shader.Free();

    {
        Window* window = GetMainWindow();
        // window->setRelativeMouse(true);

        PG::Input::PollEvents();

        Camera camera( glm::vec3( 0, 0, 3 ), glm::vec3( 0 ) );

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

            RenderSystem::Render( nullptr );

            window->EndFrame();
        }
    }

    PG::EngineQuit();

    return 0;
}
