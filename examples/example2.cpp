#include "progression.h"

int main(int argc, char* argv[]) {
    auto& conf = PG::config::Config("C:/Users/Tyler/Documents/Progression/configs/default.yaml");
    PG::EngineInitialize(conf);

    PG::Scene scene;

    PG::Camera camera = PG::Camera(PG::Transform(glm::vec3(0, 0, 5)));
    camera.AddComponent<PG::UserCameraComponent>(new PG::UserCameraComponent(&camera));

    PG::Light pointLight1(PG::Light::Type::POINT, glm::vec3(0, 0, 5), glm::vec3(1), 10);

    PG::Model* model = PG::ResourceManager::LoadModel("models/single_cube_red_mat.obj");

    PG::GameObject* gameObj = new PG::GameObject;
    gameObj->AddComponent<PG::ModelRenderer>(new PG::ModelRenderer(gameObj, model));

    scene.AddLight(&pointLight1);
    scene.AddCamera(&camera);
    scene.AddGameObject(gameObj);

    PG::Window::SetRelativeMouse(true);
    PG::Input::PollEvents();

    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene.Update();
        PG::RenderSystem::Render(&scene);

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}