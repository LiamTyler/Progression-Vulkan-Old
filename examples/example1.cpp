#include "progression.hpp"
#include <iomanip>

using namespace Progression;

int main(int argc, char* argv[]) {
	auto conf = PG::config::Config(PG_ROOT_DIR "configs/default.toml");
	if (!conf) {
        LOG_ERR("Failed to load the config file");
        exit(EXIT_FAILURE);
    }

	PG::EngineInitialize(conf);

    LOG("debug message");
    LOG_WARN("warn message p1", "p2", "p3");
    LOG_ERR("error message");

	auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/scene1.pgscn");
    if (!scene) {
        LOG_ERR("Failed to load scene:");
        exit(EXIT_FAILURE);
    }
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

    skybox.reset();
    delete scene;

    PG::EngineQuit();

    return 0;
}
