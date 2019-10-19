#include "progression.hpp"
#include <future>
#include <iomanip>
#include <thread>
#include <array>

using namespace Progression;
using namespace Progression::Gfx;

struct Position {
    float x;
    float y;

    Position( float a, float b ) : x(a), y(b) {}
    Position() = default;
};

std::ostream& operator<<(std::ostream& out, const Position& p )
{
    return out << p.x << " " << p.y;
}

int main( int argc, char* argv[] )
{
    sol::state lua;
	lua.open_libraries( sol::lib::base );

    RegisterTypesAndFunctionsToLua( lua );

	sol::usertype< Position > position_type = lua.new_usertype< Position >( "Position", sol::constructors< Position( float, float ) >() );
    position_type["x"] = &Position::x;
	position_type["y"] = &Position::y;

    REGISTER_COMPONENT_WITH_ECS( lua, Position, static_cast< Position&( entt::registry::* )( const entt::entity, float&&, float&& )> ( &entt::registry::assign< Position, float, float> ) );

    entt::registry registry;
    lua["reg"] = &registry;

    
    for ( int i = 0; i < 4; ++i )
    {
        auto e = registry.create();
        registry.assign< Position >( e );
    }

    std::cout << std::endl << "Entities: " << std::endl;
    registry.view< Position >().each( []( auto& pos )
    {
        std::cout << "entity " << pos << std::endl;
    });
    std::cout << std::endl;

    lua.script_file( PG_ROOT_DIR "script.lua");
    // registry.view<Position, Velocity>().each( lua["fn"] );

    std::cout << std::endl << "Entities: " << std::endl;
    registry.view< Position >().each([]( auto& pos )
    {
        std::cout << "entity " << pos << std::endl;
    });
    std::cout << std::endl;

    if ( !PG::EngineInitialize() )
    {
        std::cout << "Failed to initialize the engine" << std::endl;
        return 1;
    }

    {
        Window* window = GetMainWindow();
        // window->setRelativeMouse(true);
        
        // Scene* scene = Scene::Load( PG_RESOURCE_DIR "scenes/scene1.txt" );

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

            lua.script( "print( Input.GetKeyDown( Key.A ) )" );
            std::cout << "C++: " << PG::Input::GetKeyDown( Key::A ) << std::endl;
            std::this_thread::sleep_for( std::chrono::milliseconds( 1000 ) );

            // RenderSystem::Render( scene );

            window->EndFrame();
        }
    }

    PG::EngineQuit();
  

    return 0;
}
