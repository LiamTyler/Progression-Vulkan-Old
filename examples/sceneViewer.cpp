#include "progression.hpp"
#include "components/animation_component.hpp"
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
    window->SetRelativeMouse(true);
    
    Scene* scene = Scene::Load( argv[1] );
    if ( !scene )
    {
        LOG_ERR( "Could not load scene '", argv[1], "'" );
        PG::EngineQuit();
        return 0;
    }

    std::string fbxFile = PG_RESOURCE_DIR "dragon/Dragon_Baked_Actions_fbx_7.4_binary.fbx";
    // std::string fbxFile = PG_RESOURCE_DIR "primitive.fbx";
    //std::string fbxFile = PG_RESOURCE_DIR "boblampclean.md5mesh";
    
    std::vector< Animation > animations;
    std::shared_ptr< SkinnedModel > skinnedModel = std::make_shared< SkinnedModel >();
    if ( !SkinnedModel::LoadFBX( fbxFile, skinnedModel, animations ) )
    {
        LOG_ERR( "Could not load the fbx file '", fbxFile, "'" );
        PG::EngineQuit();
        return 0;
    }

    auto entity             = scene->registry.create();
    auto& transform         = scene->registry.assign< Transform >( entity );
    transform.position      = glm::vec3( 0, 0, 0 );
    // transform.rotation      = glm::vec3( glm::radians( -90.0f ), glm::radians( 90.0f ), 0 );
    transform.scale         = glm::vec3( .001f );

    //transform.position      = glm::vec3( 0, 0, 0 );
    // transform.rotation      = glm::vec3( glm::radians( 90.0f ), glm::radians( 0.0f ), 0 );
    // transform.scale         = glm::vec3( .01f );
    auto& skinned_renderer  = scene->registry.assign< SkinnedRenderer >( entity );
    skinned_renderer.model  = skinnedModel;
    LOG( "AABB min = ", skinnedModel->aabb.min );
    LOG( "AABB max = ", skinnedModel->aabb.max );
    // for ( auto& mat : skinnedModel->materials )
    // {
    //     mat->Kd = glm::vec3( 0, 1, 0 );
    // }

    auto& animator = scene->registry.assign< Animator >( entity );
    animator.animation = &animations[0];
    animator.animationTime = 0;
    animator.model = skinnedModel.get();

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

    vkDeviceWaitIdle( PG::Gfx::g_renderState.device.GetHandle() );
    scene->registry.view< SkinnedRenderer >().each([]( SkinnedRenderer& renderer ) { renderer.model->Free( false, true ); } );

    delete scene;

    PG::EngineQuit();

    return 0;
}
