#include "progression.h"
#include <chrono>

using namespace Progression;
using namespace std;

// argv[1] = path of the root directory
int main(int argc, char* argv[]) {
    srand(time(NULL));

    auto& conf = PG::config::Config(PG_ROOT_DIR "configs/default.toml");
    PG::EngineInitialize(conf);

	auto scene = Scene::Load(PG_RESOURCE_DIR "scenes/test.pgscn");
	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));

	GameObject* obj = new GameObject(Transform(glm::vec3(0), glm::vec3(glm::radians(-90.0f), glm::radians(90.0f), 0), glm::vec3(12)));
	auto time = Time::getTimePoint();
	auto model = ResourceManager::LoadModel("models/chalet.pgModel");
	auto dur = Time::getDuration(time);
	cout << "Load time: " << dur << endl;

	obj->AddComponent<ModelRenderer>(new ModelRenderer(obj, model.get()));
	scene->AddGameObject(obj);

	Window::SetRelativeMouse(true);
	PG::Input::PollEvents();
    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

		scene->Update();


		RenderSystem::Render(scene);
        
        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}