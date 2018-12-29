#include "progression.h"
#include <iomanip>

using namespace Progression;

int main(int argc, char* argv[]) {
	srand(time(NULL));

	auto conf = PG::config::Config(PG_ROOT_DIR "configs/default.toml");
	if (!conf) {
		std::cout << "could not parse config file" << std::endl;
		exit(0);
	}

	PG::EngineInitialize(conf);

	auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/scene1.pgscn");
	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));

	auto skybox = scene->getSkybox();

	Window::SetRelativeMouse(true);
	PG::Input::PollEvents();

	graphics::BindFrameBuffer(0);
	// Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene->Update();
		RenderSystem::Render(scene);
		skybox->Render(*camera);

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}
