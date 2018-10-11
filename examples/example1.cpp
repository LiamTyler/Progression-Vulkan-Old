#include "progression.h"

using namespace Progression;

std::string rootDirectory;

int main(int argc, char* argv[]) {
    srand(time(NULL));

    rootDirectory = "C:/Users/Tyler/Documents/Progression/";


    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::EngineInitialize(conf);

    Scene scene;

    Camera camera = Camera(Transform(
        glm::vec3(0, 0, 5),
        glm::vec3(0),
        glm::vec3(1)));
    camera.AddComponent<UserCameraComponent>(new UserCameraComponent(&camera));


    Light directionalLight(Light::Type::DIRECTIONAL);
    directionalLight.transform.rotation = glm::vec3(-glm::radians(35.0f), glm::radians(220.0f), 0);

    GameObject chalet(Transform(glm::vec3(0), glm::vec3(glm::radians(-90.0f), 0, 0), glm::vec3(1)));

    auto timer = Time::getTimePoint();
    
    auto chaletModel = ResourceManager::LoadModel("models/chalet.obj", false);
    std::cout << "Chalet Model Info:" << std::endl;
    std::cout << "Num meshes: " << chaletModel->meshes.size() << std::endl;
    std::cout << "Num mats: " << chaletModel->materials.size() << std::endl;
    for (const auto& mesh : chaletModel->meshes) {
        static int i = 0;
        std::cout << "Mesh " << i << ": " << std::endl;
        std::cout << "\tNumVerts: " << mesh->numVertices << std::endl;
        std::cout << "\tNumTriangles: " << mesh->numTriangles << std::endl;
        i++;
    }
    
    

    
    // auto chaletModel = ResourceManager::LoadModel("models/chalet.pgModel", false);

    std::cout << "Loading model time: " << Time::getDuration(timer) << std::endl;

    chalet.AddComponent<ModelRenderer>(new ModelRenderer(&chalet, chaletModel.get()));
    auto modelRenderComponent = chalet.GetComponent<ModelRenderer>();
    std::cout << "added model render component" << std::endl;

    scene.AddCamera(&camera);
    scene.AddLight(&directionalLight);
    scene.AddGameObject(&chalet);

    // Note: After changing the input mode, should poll for events again
    Window::SetRelativeMouse(true);
    PG::Input::PollEvents();

    glEnable(GL_DEPTH_TEST);

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