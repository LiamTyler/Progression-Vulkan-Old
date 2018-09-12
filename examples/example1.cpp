#include "progression.h"

using namespace Progression;

std::string rootDirectory;

class LightBallComponent : public Component {
public:
    LightBallComponent(GameObject* obj, GameObject* _ball) :
        Component(obj),
        ball(_ball)
    {
    }

    ~LightBallComponent() = default;
    void Start() {}
    void Stop() {}

    void Update() {
        gameObject->transform = ball->transform;
    }


    GameObject* ball;
};

class BounceComponent : public Component {
public:
    BounceComponent(GameObject* obj, const glm::vec3 startVel = glm::vec3(0, 0, 0)) :
        Component(obj),
        velocity(startVel)
    {
    }

    ~BounceComponent() = default;
    void Start() {}
    void Stop() {}

    void Update() {
        float dt = 1.0f / 30.0f;
        //float dt = Time::deltaTime();
        velocity.y += -9.81f * dt;
        gameObject->transform.position += velocity * dt;
        if (gameObject->transform.position.y < gameObject->transform.scale.x) {
            gameObject->transform.position.y = gameObject->transform.scale.x;
            velocity.y *= -.97;
        }
    }

    glm::vec3 velocity;
};

int main(int argc, char* argv[]) {
    srand(time(NULL));

    rootDirectory = "C:/Users/Tyler/Documents/Progression/";


    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::EngineInitialize(conf);

    Scene scene;

    Camera camera = Camera(Transform(
        glm::vec3(-20, 15, -25),
        glm::vec3(glm::radians(-20.0f), glm::radians(-135.0f), 0),
        glm::vec3(1)));
    camera.AddComponent<UserCameraComponent>(new UserCameraComponent(&camera));

    Material* metalMaterial = new Material(
        glm::vec3(0),
        glm::vec3(1, 1, 1),
        glm::vec3(.1),
        50,
        new Texture(new Image(rootDirectory + "resources/textures/metal.jpg")),
        ResourceManager::GetShader("default-mesh")
    );

    Material* metalMaterial2 = new Material(
        glm::vec3(0),
        glm::vec3(1, 1, 1),
        glm::vec3(.1),
        50,
        new Texture(new Image(rootDirectory + "resources/textures/metal2.jpg")),
        ResourceManager::GetShader("default-mesh")
    );

    Model* ballModel = ResourceManager::LoadModel("models/UVSphere.obj");
    ballModel->meshMaterialPairs[0].second = metalMaterial;

    Model* planeModel = ResourceManager::LoadModel("models/plane.obj");
    planeModel->meshMaterialPairs[0].second = metalMaterial2;

    for (int x = 0; x < 20; x++) {
        for (int z = 0; z < 20; z++) {
            float randHeight = 3 + 2 * (rand() / static_cast<float>(RAND_MAX));
            glm::vec3 pos = glm::vec3(-20 + 2 * x, randHeight, -20 + 2 * z);
            GameObject* ballObj = new GameObject(Transform(pos, glm::vec3(0), glm::vec3(.5)));
            ballObj->AddComponent<ModelRenderer>(new ModelRenderer(ballObj, ballModel));
            ballObj->AddComponent<BounceComponent>(new BounceComponent(ballObj));

            glm::vec3 randColor = glm::vec3((rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)));
            PG::Light* pl = new Light(PG::Light::Type::POINT, pos, randColor, 2);
            pl->AddComponent<LightBallComponent>(new LightBallComponent(pl, ballObj));

            scene.AddGameObject(ballObj);
            scene.AddLight(pl);
        }
    }


    GameObject* planeObj = new GameObject(Transform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(25)));
    planeObj->AddComponent<ModelRenderer>(new ModelRenderer(planeObj, planeModel));

    Skybox* skybox = ResourceManager::LoadSkybox({ "water/right.jpg", "water/left.jpg", "water/top.jpg", "water/bottom.jpg", "water/front.jpg", "water/back.jpg" });

    Light directionalLight(Light::Type::DIRECTIONAL);
    directionalLight.transform.rotation = glm::vec3(-glm::radians(35.0f), glm::radians(220.0f), 0);

    scene.AddCamera(&camera);
    // scene.AddLight(&directionalLight);
    scene.AddGameObject(planeObj);

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
        skybox->Render(camera);

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}