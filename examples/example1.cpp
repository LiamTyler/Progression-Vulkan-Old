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

    void Start() {
    }

    void Update() {
        gameObject->transform = ball->transform;
    }

    void Stop() {

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

    void Start() {
    }

    void Update() {
        float dt = 1.0f / 60.0f;
        //float dt = Time::deltaTime();
        velocity.y += -9.81f * dt;
        gameObject->transform.position += velocity * dt;
        if (gameObject->transform.position.y < gameObject->transform.scale.x) {
            gameObject->transform.position.y = gameObject->transform.scale.x;
            velocity.y *= -.95;
        }
    }

    void Stop() {

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
    std::cout << "\ndeferred combine:\n" << std::endl;
    Shader combineShader = Shader(rootDirectory + "resources/shaders/deferred_combine.vert", rootDirectory + "resources/shaders/deferred_combine.frag");
    combineShader.AddUniform("lights");

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
            PG::Light* pl = new Light(PG::Light::Type::POINT, glm::vec3(0, 5, 5), randColor, 2);
            pl->AddComponent<LightBallComponent>(new LightBallComponent(pl, ballObj));

            scene.AddGameObject(ballObj);
            scene.AddLight(pl);
        }
    }


    GameObject* planeObj = new GameObject(Transform(glm::vec3(0, 0, 0), glm::vec3(0), glm::vec3(50)));
    planeObj->AddComponent<ModelRenderer>(new ModelRenderer(planeObj, planeModel));

    Skybox skybox(
        rootDirectory + "resources/textures/skybox/water/right.jpg",
        rootDirectory + "resources/textures/skybox/water/left.jpg",
        rootDirectory + "resources/textures/skybox/water/top.jpg",
        rootDirectory + "resources/textures/skybox/water/bottom.jpg",
        rootDirectory + "resources/textures/skybox/water/front.jpg",
        rootDirectory + "resources/textures/skybox/water/back.jpg"
    );

    Light directionalLight(Light::Type::DIRECTIONAL);
    directionalLight.transform.rotation = glm::vec3(-glm::radians(35.0f), glm::radians(220.0f), 0);


    scene.AddCamera(&camera);
    scene.AddLight(&directionalLight);
    // scene.AddLight(&pointLight1);
    // scene.AddGameObject(ballObj);
    scene.AddGameObject(planeObj);


    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gDiffuse, gSpecularExp;

    // - position color buffer 
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, Window::getWindowSize().x, Window::getWindowSize().y, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    // - normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, Window::getWindowSize().x, Window::getWindowSize().y, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

    // - diffuse color buffer
    glGenTextures(1, &gDiffuse);
    glBindTexture(GL_TEXTURE_2D, gDiffuse);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Window::getWindowSize().x, Window::getWindowSize().y, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gDiffuse, 0);

    // - specular color + specular exp buffer
    glGenTextures(1, &gSpecularExp);
    glBindTexture(GL_TEXTURE_2D, gSpecularExp);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Window::getWindowSize().x, Window::getWindowSize().y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gSpecularExp, 0);


    // - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
    glDrawBuffers(4, attachments);

    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Window::getWindowSize().x, Window::getWindowSize().y);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "frame buffer incomplete" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

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

        RenderSystem::UpdateLights(&scene, &camera);


        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

        const auto& bgColor = scene.GetBackgroundColor();
        // glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        deferredShader.Enable();

        RenderSystem::UploadCameraProjection(deferredShader, camera);

        const auto& gameObjects = scene.GetGameObjects();
        // std::cout << gameObjects.size() << std::endl;
        for (const auto& obj : scene.GetGameObjects()) {
            glm::mat4 M = obj->transform.GetModelMatrix();
            glm::mat4 MV = camera.GetV() * M;
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
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        
        combineShader.Enable();
        RenderSystem::UploadLights(combineShader);
        glBindVertexArray(quadVAO);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gDiffuse);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, gSpecularExp);
        glUniform1i(combineShader["gPosition"], 0);
        glUniform1i(combineShader["gNormal"], 1);
        glUniform1i(combineShader["gDiffuse"], 2);
        glUniform1i(combineShader["gSpecularExp"], 3);

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glBlitFramebuffer(0, 0, Window::getWindowSize().x, Window::getWindowSize().y, 0, 0, Window::getWindowSize().x, Window::getWindowSize().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        skybox.Render(camera);

        //gameObj->transform.position.y += 3;
        //RenderSystem::Render(&scene);
        //gameObj->transform.position.y -= 3;


        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}
