#include "progression.h"
#include <iomanip>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Progression;
using namespace std;

/*
void Update() {
    float dt = Time::deltaTime();
    glm::vec3 toGoal = glm::normalize(goal - gameObject->transform.position);
    velocity += dt * toGoal;
    float speed = glm::length(velocity);
    if (speed < 2)
        velocity = speed * glm::normalize(velocity);
    gameObject->transform.position += velocity * dt;

    if (glm::length(goal - gameObject->transform.position) < .1) {
        goal.x *= -1;
        goal.z *= -1;
    }
}
*/

float E(float t) {
    return 

float ttc(const glm::vec3& pa, const glm::vec3& pb, const glm::vec3& va,
        const glm::vec3& vb, float ra, float rb)
{
    float maxt = 999;
    glm::vec3 pDiff = pb - pa;

    glm::vec3 relativeVelocity = vb - va;
    float a = glm::dot(relativeVelocity, relativeVelocity);
    float b = 2 * glm::dot(relativeVelocity, pDiff);
    float c = glm::dot(pDiff, pDiff) - pow(ra + rb, 2.0);

    float det = b*b - 4*a*c;
    float t1 = maxt, t2 = maxt;
    if (det > 0) {
        t1 = (-b + sqrt(det)) / (2 * a);
        t2 = (-b - sqrt(det)) / (2 * a);
    }

    float t = std::min(t1, t2);
    if (t < 0 && std::max(t1, t2) > 0) t = 100;
    if (t < 0 || t > maxt) t = maxt;

    return t;
}

class TTCcomponent : public Component {
    public:
        TTCcomponent(GameObject* g) :
            Component(g),
            velocity(0),
            goal(0)
        {
        }

        ~TTCcomponent() = default;

        void Start() {}
        void Stop() {}
        
        void Update() {
        }

        void PostUpdate() {
            velocity += Time::deltaTime() * a;
            gameObject->transform.position += velocity * Time::deltaTime();
            if (glm::length(goal - gameObject->transform.position) < .1) {
                goal.x *= -1;
                goal.z *= -1;
            }
        }

        float radius;
        glm::vec3 a;
        glm::vec3 velocity;
        glm::vec3 goalVelocity;
        glm::vec3 goal;
        std::vector<TTCcomponent*> ttc;
};

int main(int argc, char* argv[]) {
    srand(time(NULL));

    auto conf = PG::config::Config(PG_ROOT_DIR "configs/default.toml");
    if (!conf) {
        std::cout << "could not parse config file" << std::endl;
        exit(0);
    }

    PG::EngineInitialize(conf);

    auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/crowd.pgscn");

    auto camera = scene->GetCamera();
    camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));
    auto skybox = scene->getSkybox();
    auto robotModel = ResourceManager::GetModel("robot");
    cout << "robot meshes: " << robotModel->meshes.size() << endl;
    for (auto& mesh : robotModel->meshes)
        cout << "triangles: " << mesh->numTriangles << endl;
    int SIZE = 10;
    for (int r = 0; r < 10; ++r) {
        for (int c = 0; c < SIZE; ++c) {
            Transform t(3.0f * glm::vec3(-SIZE + 2 * c, .1, SIZE - 2*r), glm::vec3(0), glm::vec3(16));
            auto g = new GameObject(t);
            g->AddComponent<ModelRenderer>(new ModelRenderer(g, robotModel.get()));
            g->AddComponent<TTCcomponent>(new TTCcomponent(g));
            g->GetComponent<TTCcomponent>()->goal = glm::vec3(50, .3, -50);
            scene->AddGameObject(g);
        }
    }

    Window::SetRelativeMouse(true);
    PG::Input::PollEvents();

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
