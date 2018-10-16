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
    directionalLight.transform.rotation = glm::vec3(glm::radians(0.0f), glm::radians(0.0f), 0);

    // GameObject chalet(Transform(glm::vec3(0), glm::vec3(glm::radians(0.0f), 0, 0), glm::vec3(5)));
    GameObject gameObj(Transform(glm::vec3(0, 0, 0), glm::vec3(glm::radians(0.0f), glm::radians(0.0f), 0), glm::vec3(1)));
    
    auto model = ResourceManager::LoadModel("models/cube.obj", false);
    std::cout << "Model Info:" << std::endl;
    std::cout << "Num meshes: " << model->meshes.size() << std::endl;
    std::cout << "Num mats: " << model->materials.size() << std::endl;
    for (const auto& mesh : model->meshes) {
        static int i = 0;
        std::cout << "Mesh " << i << ": " << std::endl;
        std::cout << "\tNumVerts: " << mesh->numVertices << std::endl;
        std::cout << "\tNumTriangles: " << mesh->numTriangles << std::endl;
        i++;
    }
    model->materials[0]->diffuse = glm::vec3(.7, 0, 0);
    model->materials[0]->specular = glm::vec3(.7);
    model->materials[0]->shininess = 60;

    gameObj.AddComponent<ModelRenderer>(new ModelRenderer(&gameObj, model.get()));
    auto modelRenderComponent = gameObj.GetComponent<ModelRenderer>();

    Light pointLight(Light::Type::POINT, Transform(gameObj.transform.position), glm::vec3(0, 0, 1), 1);

    GameObject floor(Transform(glm::vec3(0, -2, 0), glm::vec3(0), glm::vec3(20, 1, 20)));
    //floor.AddComponent<ModelRenderer>(new ModelRenderer(&floor, ResourceManager::GetModel("plane").get()));

    // scene.AddGameObject(&floor);
    scene.AddCamera(&camera);
    scene.AddLight(&directionalLight);
    // scene.AddLight(&pointLight);
    scene.AddGameObject(&gameObj);
    
    /*

    std::cout << "before load scene" << std::endl;
    auto scene = Scene::Load(rootDirectory + "resources/scenes/glowSphereScene.pgscn");
    std::cout << "A" << std::endl;

    auto camera = scene->GetCamera();
    camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 15));
    std::cout << "B" << std::endl;

    auto planeModel = ResourceManager::GetModel("metalFloor");
    GameObject* planeObj = scene->GetGameObject("floor");
    planeObj->AddComponent<ModelRenderer>(new ModelRenderer(planeObj, planeModel.get()));
    std::cout << "C" << std::endl;
    */

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