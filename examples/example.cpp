#include "progression.hpp"
#include <iomanip>
#include <thread>
#include <future>

#ifdef __linux__ 
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

using namespace Progression;

int main(int argc, char* argv[]) {
    PG_UNUSED(argc);
    PG_UNUSED(argv);

    PG::EngineInitialize();

    Window* window = getMainWindow();
    //window->setRelativeMouse(true);

    Scene* scene = Scene::load(PG_RESOURCE_DIR "scenes/scene1.txt");

    PG::Input::PollEvents();


    // Game loop
    while (!PG::EngineShutdown) {
        window->startFrame();
        PG::Input::PollEvents();

        ResourceManager::update();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC)) {
            PG::EngineShutdown = true;
        }

        RenderSystem::render(scene);

        window->endFrame();
    }

    PG::EngineQuit();

    return 0;
}
