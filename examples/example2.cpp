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

glm::vec2 dE(const glm::vec2& pa, const glm::vec2& pb, const glm::vec2& va,
        const glm::vec2& vb, float ra, float rb)
{
    float maxt  = 999;
    auto pDiff  = pb - pa;
    auto vDiff  = va - vb;
    // cout << "pDiff= " << pDiff << " vDiff = " << vDiff << endl;
    auto radius = ra + rb;
    auto dist = glm::length(pDiff);
    if (radius > dist)
        radius = .99 * dist;

    float a = glm::dot(vDiff, vDiff);
    float b = glm::dot(vDiff, pDiff);
    float c = glm::dot(pDiff, pDiff) - radius * radius;

    float disc = b*b - a*c;
    // cout << "disc = " << disc << ", a = " << a << ", b = " << b << ", c = " << c << endl;
    if (disc < 0 || abs(a) < 0.001)
        return glm::vec2(0);
    
    disc = sqrt(disc);
    float t = (b - disc) /  a;
    if (t < 0 || t > maxt)
        return glm::vec2(0);

    float k = 10.5;
    float m = 2.0;
    float t0 = 3;
    return k * exp(-t / t0)*(vDiff - (vDiff*b - pDiff*a)/disc)/(a*pow(t, m))*(m/t + 1.0f /t0);
}

class TTCcomponent : public Component {
    public:
        TTCcomponent(GameObject* g, Scene* sc) :
            Component(g),
            scene(sc),
            radius(1.5),
            force(0),
            velocity(0),
            goalVelocity(0),
            goal(0)
        {
            goal = 50.0f * glm::vec2(2.0f * (rand() / (float) RAND_MAX) - 1.0f, 2.0f * (rand() / (float) RAND_MAX) - 1.0f);
        }

        ~TTCcomponent() = default;

        void Start() {}
        void Stop() {}
        
        void Update() {
            glm::vec2 pos = glm::vec2(gameObject->transform.position.x, gameObject->transform.position.z);
            // cout << "pos = " << pos << endl;
            goalVelocity = 6.0f * glm::normalize(goal - pos);
            
            scene->GetNeighbors(gameObject, 10, neighbors);
            force = 2.0f * (goalVelocity - velocity);
            force += glm::vec2(2.0f * (rand() / (float) RAND_MAX) - 1.0f, 2.0f * (rand() / (float) RAND_MAX) - 1.0f);

            for (auto& neighbor : neighbors) {
                auto ttc = neighbor->GetComponent<TTCcomponent>();
                if (ttc) {
                    glm::vec2 nPos = glm::vec2(neighbor->transform.position.x, neighbor->transform.position.z);
                    // cout << "nPos = " << nPos << endl;
                    auto diff = pos - nPos;
                    float dist = glm::length(diff);
                    float r = radius;
                    if (dist < 2 * r)
                        r = dist / 2.001;

                    auto dEdx = dE(pos, nPos, velocity, ttc->velocity, radius, ttc->radius);
                    // std::cout << dEdx << endl;
                    auto FAvoid = -dEdx;
                    float mag = glm::length(FAvoid);
                    float maxF = 15;
                    if (mag > maxF)
                        FAvoid = maxF * FAvoid / mag;

                    force += FAvoid;
                }
            }
        }

        void PostUpdate() {
            float dt = Time::deltaTime();
            velocity += force * dt;
            auto dP = velocity * dt;
            gameObject->transform.position += glm::vec3(dP.x, 0, dP.y);
            auto xzPos = glm::vec2(gameObject->transform.position.x, gameObject->transform.position.z);
            if (glm::length(goal - xzPos) < .1) {
                goal.x *= -1;
                goal.y *= -1;
            }
        }

        Scene* scene;
        float radius;
        glm::vec2 force;
        glm::vec2 velocity;
        glm::vec2 goalVelocity;
        glm::vec2 goal;
        std::vector<GameObject*> neighbors;
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
            Transform t(3.0f * glm::vec3(-20 + 4 * c, .1, 20 - 4*r), glm::vec3(0), glm::vec3(16));
            auto g = new GameObject(t);
            g->AddComponent<ModelRenderer>(new ModelRenderer(g, robotModel.get()));
            g->AddComponent<TTCcomponent>(new TTCcomponent(g, scene));
            // g->GetComponent<TTCcomponent>()->goal = glm::vec2(50, -50);
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
        auto& gameObjects = scene->GetGameObjects();
        for (auto& obj : gameObjects) {
            auto ttc = obj->GetComponent<TTCcomponent>();
            if (ttc) {
                ttc->PostUpdate();
            }
        }

        RenderSystem::Render(scene);
        skybox->Render(*camera);

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}
