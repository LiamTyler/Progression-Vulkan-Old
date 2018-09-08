#include "progression.h"

using namespace Progression;

std::string rootDirectory;

// argv[1] = path of the root directory
int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Please pass in the path of the root directory as the first argument" << std::endl;
        return 0;
    }
    rootDirectory = argv[1];

    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::EngineInitialize(conf);

    Scene scene;

    Camera camera = Camera(Transform(
        glm::vec3(0, 0, 5),
        glm::vec3(0, 0, 0),
        glm::vec3(1)));
    camera.AddComponent<UserCameraComponent>(new UserCameraComponent(&camera));

    //Light directionalLight(Light::Type::DIRECTIONAL);
    //directionalLight.transform.rotation = glm::vec3(-glm::radians(45.0f), glm::radians(45.0f), 0);
    //scene.AddLight(&directionalLight);

    Light pointLight1(Light::Type::POINT);
    pointLight1.transform.position = glm::vec3(0, 0, 5);
    pointLight1.intensity = 5;
    scene.AddLight(&pointLight1);

    scene.AddCamera(&camera);
    
    // Shader& phongShader = *ResourceManager::GetShader("default-mesh");
    Model* model = ResourceManager::LoadModel("models/single_cube_red_mat.obj");
    if (!model) {
        std::cout << "no model" << std::endl;
        exit(EXIT_FAILURE);
    }

    GameObject* gameObj = new GameObject(
        Transform(
            glm::vec3(0),
            glm::vec3(0),
            glm::vec3(1)
        ));

    gameObj->AddComponent<ModelRenderer>(new ModelRenderer(gameObj, model));

    // Note:: After changing the input mode, should poll for events again
    Window::SetRelativeMouse(true);
    PG::Input::PollEvents();

    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene.Update();
        RenderSystem::Render(&scene);

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}
