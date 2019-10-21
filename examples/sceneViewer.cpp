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

    LOG( "Scene backgroundColor: ", scene->backgroundColor );
    LOG( "Scene ambientColor: ", scene->ambientColor );
    LOG( "Scene Camera: " );
    LOG( "\tposition: ", scene->camera.position );
    LOG( "\trotation: ", scene->camera.rotation );
    LOG( "\tfov: ", scene->camera.fov );
    LOG( "\taspectRatio: ", scene->camera.aspectRatio );
    LOG( "\tnearPlane: ", scene->camera.nearPlane );
    LOG( "\tfarPlane: ", scene->camera.farPlane );
    LOG( "Scene Directional Light: " );
    LOG( "\tcolorAndIntensity: ", scene->directionalLight.colorAndIntensity );
    LOG( "\tdirection: ", scene->directionalLight.direction );
    LOG( "Num PointLights: ", scene->pointLights.size() );
    for ( size_t i = 0; i < scene->pointLights.size(); ++i )
    {
        const auto& l = scene->pointLights[i];
        LOG( "\tPointLight[", i, "].colorAndIntensity = ", l.colorAndIntensity );
        LOG( "\tPointLight[", i, "].position = ", l.position );
        LOG( "\tPointLight[", i, "].radius = ", l.radius );
    }
    LOG( "Num SpotLights: ", scene->spotLights.size() );
    for ( size_t i = 0; i < scene->spotLights.size(); ++i )
    {
        const auto& l = scene->spotLights[i];
        LOG( "\tspotLight[", i, "].colorAndIntensity = ", l.colorAndIntensity );
        LOG( "\tspotLight[", i, "].position = ", l.position );
        LOG( "\tspotLight[", i, "].radius = ", l.radius );
        LOG( "\tspotLight[", i, "].direction = ", l.direction );
        LOG( "\tspotLight[", i, "].cutoff = ", l.cutoff );
    }

    LOG( "All entities: " );
    scene->registry.each([]( const auto& e )
    {
        LOG( "Entity ", (uint32_t) e );
    });

    LOG( "Components: " );
    scene->registry.view< Transform, NameComponent, EntityMetaData >().each([]( const auto& e, auto& t, auto& n, auto& m  )
    {
        LOG( "Entity ", (uint32_t) e );
        LOG( "\tTransform.position = ", t.position );
        LOG( "\tTransform.rotation = ", t.rotation );
        LOG( "\tTransform.scale    = ", t.scale );
        LOG( "\tTransform.scale    = ", t.scale );
        LOG( "\tname = ", n.name );
        LOG( "\tmeta.parent   = ", (uint32_t) m.parent );
        LOG( "\tmeta.isStatic = ", m.isStatic );
    });
  

    ScriptCreateInfo info;
    info.name = "updater";
    info.filename = PG_ROOT_DIR "script.lua";

    std::shared_ptr< Script > script = std::make_shared< Script >();
    script->Load( &info );

    std::shared_ptr< Script > script2 = std::make_shared< Script >();
    info.name = "single";
    info.filename = PG_ROOT_DIR "script2.lua";
    script2->Load( &info );

    entt::entity e = GetEntityByName( scene->registry, "entity1" );
    auto& comp = scene->registry.assign< ScriptComponent >( e );
    comp.AddScript( script );

    e = GetEntityByName( scene->registry, "entity2" );
    auto& comp2 = scene->registry.assign< ScriptComponent >( e );
    comp2.AddScript( script );
    comp2.AddScript( script2 );


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

        scene->Update();
        // RenderSystem::Render( scene );

        std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

        window->EndFrame();
    }

    delete scene;

    PG::EngineQuit();

    return 0;
}
