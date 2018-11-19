#include "progression.h"
#include <iomanip>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Progression;
using namespace std;

int main(int argc, char* argv[]) {
	srand(time(NULL));

	auto conf = PG::config::Config(PG_ROOT_DIR "configs/default.toml");
	if (!conf) {
		std::cout << "could not parse config file" << std::endl;
		exit(0);
	}

	PG::EngineInitialize(conf);

	auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/scene1.pgscn");

    auto skybox = scene->getSkybox();

	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));

    auto aFile = AudioSystem::LoadAudio("/home/liam/Documents/audio/tone.wav");
    if (!aFile) {
        cout << "Could not load wav file" << endl;
        return 0;
    }
    auto source = AudioSystem::AddSource("default");
    source->setAudio(aFile);

    AudioSystem::PlayAll();

	// Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;
        if (PG::Input::GetKeyDown(PG::PG_K_I))
            AudioSystem::PlayAll();
        if (PG::Input::GetKeyDown(PG::PG_K_O))
            AudioSystem::PauseAll();
        if (PG::Input::GetKeyDown(PG::PG_K_P))
            AudioSystem::StopAll();
        if (PG::Input::GetKeyDown(PG::PG_K_L)) {
            if (source->isLooping())
                source->setLooping(false);
            else
                source->setLooping(true);
        }

        scene->Update();

        RenderSystem::Render(scene);

		skybox->Render(*camera);
		

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}
