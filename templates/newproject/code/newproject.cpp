#include "##projectName##.hpp"
#include "progression.hpp"
#include "configuration.hpp"

PG::Scene* scene;

namespace ##projectName## {

    bool init() {
        PG::ResourceManager::rootResourceDir = ROOT_DIR;
        scene = PG::Scene::Load(ROOT_DIR "scenes/scene1.pgscn");
        if (!scene)
            return false;
        auto camera = scene->GetCamera();
        camera->AddComponent<PG::UserCameraComponent>(new PG::UserCameraComponent(camera));

        return true;
    }

    void shutdown() {
        delete scene;
    }

    void handleInput() {
		if (PG::Input::GetKeyDown(PG::PG_K_ESC))
			PG::EngineShutdown = true;
    }

    void update() {
		scene->Update();
    }

    void render() {
        PG::RenderSystem::Render(scene);
    }
}
