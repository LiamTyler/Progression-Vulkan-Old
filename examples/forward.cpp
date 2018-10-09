#include "progression.h"
#include <chrono>

#define BLOCK_SIZE 16
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
        gameObject->boundingBox.setCenter(gameObject->transform.position);
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
        gameObject->boundingBox.setCenter(gameObject->transform.position);
    }

    glm::vec3 velocity;
};

// argv[1] = path of the root directory
int main(int argc, char* argv[]) {
    srand(time(NULL));

    rootDirectory = "C:/Users/Tyler/Documents/Progression/";

    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::EngineInitialize(conf);

    auto scene = Scene::Load(rootDirectory + "resources/scenes/scene1.pgscn");

    auto camera = scene->GetCamera();
    camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 15));

    auto ballModel = ResourceManager::GetModel("metalBall");
    auto planeModel = ResourceManager::GetModel("metalFloor");

    int X, Z, startX, startZ;
    float DX, DZ;

    bool single = false;
    bool fourH = true;
    bool oneK = true;
    bool fourK = false;
    bool tenK = true;

    if (single) {
        X = 1;
        Z = 1;
        startX = 0;
        startZ = 0;
        DX = 10;
        DZ = 10;
    }
    if (fourH) {
        X = 20;
        Z = 20;
        startX = -X;
        startZ = -Z;
        DX = 1;
        DZ = 1;
    }
    if (oneK) {
        X = 32;
        Z = 32;
        startX = -X;
        startZ = -Z;
        DX = 1;
        DZ = 1;
    }
    if (fourK) {
        X = 64;
        Z = 64;
        startX = -X;
        startZ = -Z;
        DX = 1;
        DZ = 1;
    }
    if (tenK) {
        X = 100;
        Z = 100;
        startX = -X;
        startZ = -Z;
        DX = 1;
        DZ = 1;
    }

    float intensity = 2;
    float cutOffIntensity = 0.03;
    for (float x = 0; x < X; x += DX) {
        for (float z = 0; z < Z; z += DZ) {
            float randHeight = 3 + 2 * (rand() / static_cast<float>(RAND_MAX));
            glm::vec3 pos = glm::vec3(startX + 2 * x, randHeight, startZ + 2 * z);
            GameObject* ballObj = new GameObject(Transform(pos, glm::vec3(0), glm::vec3(.5)));
            ballObj->AddComponent<ModelRenderer>(new ModelRenderer(ballObj, ballModel.get()));
            ballObj->AddComponent<BounceComponent>(new BounceComponent(ballObj));
            ballObj->boundingBox.Encompass(BoundingBox(glm::vec3(-1), glm::vec3(1)), ballObj->transform);

            glm::vec3 randColor = glm::vec3((rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)));
            PG::Light* pl = new Light(PG::Light::Type::POINT, pos, randColor, intensity);
            pl->AddComponent<LightBallComponent>(new LightBallComponent(pl, ballObj));
            float lightRadius = std::sqrtf(intensity / cutOffIntensity);
            // lightRadius = 4;
            pl->boundingBox.Encompass(BoundingBox(glm::vec3(-1), glm::vec3(1)), Transform(pl->transform.position, glm::vec3(0), glm::vec3(lightRadius)));

            scene->AddGameObject(ballObj);
            scene->AddLight(pl);
        }
    }
    std::cout << "light Radius = " << std::sqrtf(intensity / cutOffIntensity) << std::endl;
    
    GameObject* planeObj = scene->GetGameObject("floor");
    planeObj->AddComponent<ModelRenderer>(new ModelRenderer(planeObj, planeModel.get()));
    float e = 100;
    planeObj->boundingBox = BoundingBox(glm::vec3(-e, -.1, -e), glm::vec3(e, .1, e));
    
    auto skybox = scene->getSkybox();

    // Note: After changing the input mode, should poll for events again
    Window::SetRelativeMouse(true);
    PG::Input::PollEvents();

    graphics::ToggleDepthTesting(true);
    Time::Restart();

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