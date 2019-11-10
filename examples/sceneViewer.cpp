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
    window->SetRelativeMouse(true);
    
    Scene* scene = Scene::Load( argv[1] );
    if ( !scene )
    {
        LOG_ERR( "Could not load scene '", argv[1], "'" );
        PG::EngineQuit();
        return 0;
    }
    
    std::shared_ptr< Model > skinnedModel1 = std::make_shared< Model >();
    ModelCreateInfo info;
    info.name = "arms";
    info.filename = PG_RESOURCE_DIR "primitive.fbx";
    info.optimize = true;
    if ( !skinnedModel1->Load( &info ) )
    {
        LOG_ERR( "Could not load the fbx file '", info.filename, "'" );
        PG::EngineQuit();
        return 0;
    }

    {
        auto entity             = scene->registry.create();
        auto& transform         = scene->registry.assign< Transform >( entity );
        transform.position      = glm::vec3( 0, 0, 0 );
        // transform.rotation      = glm::vec3( glm::radians( -90.0f ), glm::radians( 90.0f ), 0 );
        transform.scale         = glm::vec3( .001f );

        auto& skinned_renderer  = scene->registry.assign< SkinnedRenderer >( entity );
        skinned_renderer.model  = skinnedModel1;
        skinned_renderer.materials = skinnedModel1->materials;

        // for ( auto& mat : skinnedModel->materials )
        // {
        //     mat->Kd = glm::vec3( 0, 1, 0 );
        // }

        auto& animator         = scene->registry.assign< Animator >( entity, skinnedModel1.get() );
        animator.animation     = &skinnedModel1->animations[0];
        animator.animationTime = 0;
    }

    // std::shared_ptr< Model > skinnedModel2 = skinnedModel1;
    std::shared_ptr< Model > skinnedModel2 = std::make_shared< Model >();
    info.name = "dragon";
    info.filename = PG_RESOURCE_DIR "dragon/Dragon_Baked_Actions_fbx_7.4_binary.fbx";
    // info.filename = PG_RESOURCE_DIR "models/chalet2.obj";
    if ( !skinnedModel2->Load( &info ) )
    {
        LOG_ERR( "Could not load the fbx file '", info.filename, "'" );
        PG::EngineQuit();
        return 0;
    }
    
    {
        auto entity             = scene->registry.create();
        auto& transform         = scene->registry.assign< Transform >( entity );
        transform.position      = glm::vec3( 3, 0, 0 );
        // transform.rotation      = glm::vec3( glm::radians( -90.0f ), glm::radians( 90.0f ), 0 );
        transform.scale         = glm::vec3( .001f );
    
        auto& skinned_renderer  = scene->registry.assign< SkinnedRenderer >( entity );
        skinned_renderer.model  = skinnedModel2;
        skinned_renderer.materials = skinnedModel2->materials;
    
        auto& animator         = scene->registry.assign< Animator >( entity, skinnedModel2.get() );
        animator.animation     = &skinnedModel2->animations[0];
        animator.animationTime = 0.5;
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
