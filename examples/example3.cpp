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

// argv[1] = path of the root directory
int main(int argc, char* argv[]) {
    srand(time(NULL));

    rootDirectory = "C:/Users/Tyler/Documents/Progression/";

    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::EngineInitialize(conf);

    Shader deferredShader = Shader(rootDirectory + "resources/shaders/deferred_phong.vert", rootDirectory + "resources/shaders/deferred_phong.frag");
    Shader combineShader = Shader(rootDirectory + "resources/shaders/deferred_combine.vert", rootDirectory + "resources/shaders/deferred_combine.frag");
    combineShader.AddUniform("lights");
    Shader lightVolumeShader = Shader(rootDirectory + "resources/shaders/lightVolume.vert", rootDirectory + "resources/shaders/lightVolume.frag");

    float quadVerts[] = {
        -1, 1,
        -1, -1,
        1, -1,

        -1, 1,
        1, -1,
        1, 1
    };
    GLuint quadVAO;
    GLuint quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(combineShader["vertex"]);
    glVertexAttribPointer(combineShader["vertex"], 2, GL_FLOAT, GL_FALSE, 0, 0);

    auto scene = Scene::Load(rootDirectory + "resources/scenes/scene1.pgscn");

    auto camera = scene->GetCamera();
    camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera));

    auto ballModel = ResourceManager::GetModel("metalBall");
    auto planeModel = ResourceManager::GetModel("metalFloor");

    for (int x = 0; x < 20; x++) {
        for (int z = 0; z < 20; z++) {
            float randHeight = 3 + 2 * (rand() / static_cast<float>(RAND_MAX));
            glm::vec3 pos = glm::vec3(-20 + 2 * x, randHeight, -20 + 2 * z);
            GameObject* ballObj = new GameObject(Transform(pos, glm::vec3(0), glm::vec3(.5)));
            ballObj->AddComponent<ModelRenderer>(new ModelRenderer(ballObj, ballModel.get()));
            ballObj->AddComponent<BounceComponent>(new BounceComponent(ballObj));

            glm::vec3 randColor = glm::vec3((rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)));
            PG::Light* pl = new Light(PG::Light::Type::POINT, pos, randColor, 2);
            pl->AddComponent<LightBallComponent>(new LightBallComponent(pl, ballObj));

            scene->AddGameObject(ballObj);
            scene->AddLight(pl);
        }
    }
    
    GameObject* planeObj = scene->GetGameObject("floor");
    planeObj->AddComponent<ModelRenderer>(new ModelRenderer(planeObj, planeModel.get()));
    
    auto skybox = scene->getSkybox();

    GLuint lightVolumeVAO = graphics::CreateVAO();
    GLuint* lightVolumeVBOs = ballModel->meshes[0]->getBuffers();

    glBindBuffer(GL_ARRAY_BUFFER, lightVolumeVBOs[0]);
    glEnableVertexAttribArray(lightVolumeShader["vertex"]);
    glVertexAttribPointer(lightVolumeShader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightVolumeVBOs[3]);

    GLuint gBuffer = graphics::CreateFrameBuffer();
    GLuint gPosition = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGB32F);
    GLuint gNormal = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGB32F);
    GLuint gDiffuse = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGB);
    GLuint gSpecularExp = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);

    graphics::AttachColorTexturesToFBO({ gPosition, gNormal, gDiffuse, gSpecularExp });

    GLuint rboDepth = graphics::CreateRenderBuffer(Window::getWindowSize().x, Window::getWindowSize().y);
    graphics::AttachRenderBufferToFBO(rboDepth);
    graphics::FinalizeFBO();

    graphics::BindFrameBuffer();

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

        RenderSystem::UpdateLights(scene, camera);

        graphics::BindFrameBuffer(gBuffer);

        graphics::SetClearColor(glm::vec4(0));
        graphics::Clear();

        deferredShader.Enable();

        RenderSystem::UploadCameraProjection(deferredShader, *camera);

        const auto& gameObjects = scene->GetGameObjects();
        // std::cout << gameObjects.size() << std::endl;
        for (const auto& obj : scene->GetGameObjects()) {
            glm::mat4 M = obj->transform.GetModelMatrix();
            glm::mat4 MV = camera->GetV() * M;
            glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
            glUniformMatrix4fv(deferredShader["modelViewMatrix"], 1, GL_FALSE, glm::value_ptr(MV));
            glUniformMatrix4fv(deferredShader["normalMatrix"], 1, GL_FALSE, glm::value_ptr(normalMatrix));
            for (const auto& mr : obj->GetComponent<ModelRenderer>()->meshRenderers) {
                glBindVertexArray(mr->vao);
                RenderSystem::UploadMaterial(deferredShader, *mr->material);
                glDrawElements(GL_TRIANGLES, mr->mesh->getNumTriangles() * 3, GL_UNSIGNED_INT, 0);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        graphics::Clear();

        // blit the deferred depth buffer to the main screen
        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, Window::getWindowSize().x, Window::getWindowSize().y, 0, 0, Window::getWindowSize().x, Window::getWindowSize().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        lightVolumeShader.Enable();

        // Turn on additive blending and disable depth writing
        graphics::ToggleBlending(true);
        graphics::ToggleCulling(true);
        graphics::ToggleDepthBufferWriting(false);

        graphics::Bind2DTexture(gPosition, lightVolumeShader["gPosition"], 0);
        graphics::Bind2DTexture(gNormal, lightVolumeShader["gNormal"], 1);
        graphics::Bind2DTexture(gDiffuse, lightVolumeShader["gDiffuse"], 2);
        graphics::Bind2DTexture(gSpecularExp, lightVolumeShader["gSpecularExp"], 3);

        glBindVertexArray(lightVolumeVAO);
        const auto& pointLights = scene->GetPointLights();
        for (const auto& light : pointLights) {

            // calculate light volume model matrix
            float cutOffIntensity = 0.02;
            float lightRadius = std::sqrtf(light->intensity / cutOffIntensity);
            lightRadius = 4;
            glm::mat4 lightModelMatrix(1);
            lightModelMatrix = glm::translate(lightModelMatrix, light->transform.position);
            lightModelMatrix = glm::scale(lightModelMatrix, glm::vec3(lightRadius));

            // calculate MVP
            glm::mat4 MVP = camera->GetP() * camera->GetV() * lightModelMatrix;

            // upload data to GPU
            glUniformMatrix4fv(lightVolumeShader["MVP"], 1, GL_FALSE, glm::value_ptr(MVP));
            glUniform3fv(lightVolumeShader["lightPos"], 1, glm::value_ptr(glm::vec3(camera->GetV() * glm::vec4(light->transform.position, 1))));
            glUniform3fv(lightVolumeShader["lightColor"], 1, glm::value_ptr(light->color * light->intensity));

            // render the light volume
            glDrawElements(GL_TRIANGLES, ballModel->meshes[0]->getNumTriangles() * 3, GL_UNSIGNED_INT, 0);
        }

        graphics::ToggleBlending(false);
        graphics::ToggleCulling(false);
        graphics::ToggleDepthBufferWriting(true);
        
        skybox->Render(*camera);

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}