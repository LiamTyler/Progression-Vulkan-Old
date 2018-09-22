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
        float dt = 1.0f / 100.0f;
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

    Shader deferredShader = Shader(rootDirectory + "resources/shaders/deferred_phong.vert", rootDirectory + "resources/shaders/deferred_phong.frag");
    Shader combineShader = Shader(rootDirectory + "resources/shaders/deferred_combine.vert", rootDirectory + "resources/shaders/deferred_combine.frag");
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
    camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 15));

    auto ballModel = ResourceManager::GetModel("metalBall");
    auto planeModel = ResourceManager::GetModel("metalFloor");

    int X = 20;
    int Z = 20;
    int DX = 4;
    int DZ = 4;
    //for (int x = 0; x < 20; x++) {
    //    for (int z = 0; z < 20; z++) {
            for (int x = 0; x < X; x += DX) {
                for (int z = 0; z < Z; z += DZ) {
            float randHeight = 3 + 2 * (rand() / static_cast<float>(RAND_MAX));
            glm::vec3 pos = glm::vec3(-X + 2 * x, randHeight, -Z + 2 * z);
            GameObject* ballObj = new GameObject(Transform(pos, glm::vec3(0), glm::vec3(.5)));
            ballObj->AddComponent<ModelRenderer>(new ModelRenderer(ballObj, ballModel.get()));
            ballObj->AddComponent<BounceComponent>(new BounceComponent(ballObj));
            ballObj->boundingBox.Encompass(BoundingBox(glm::vec3(-1), glm::vec3(1)), ballObj->transform);

            glm::vec3 randColor = glm::vec3((rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)));
            PG::Light* pl = new Light(PG::Light::Type::POINT, pos, randColor, 2);
            pl->AddComponent<LightBallComponent>(new LightBallComponent(pl, ballObj));
            float cutOffIntensity = 0.03;
            float lightRadius = std::sqrtf(pl->intensity / cutOffIntensity);
            std::cout << lightRadius << std::endl;
            // lightRadius = 4;
            pl->boundingBox.Encompass(BoundingBox(glm::vec3(-1), glm::vec3(1)), Transform(pl->transform.position, glm::vec3(0), glm::vec3(lightRadius)));

            scene->AddGameObject(ballObj);
            scene->AddLight(pl);
        }
    }
    
    GameObject* planeObj = scene->GetGameObject("floor");
    planeObj->AddComponent<ModelRenderer>(new ModelRenderer(planeObj, planeModel.get()));
    planeObj->boundingBox = BoundingBox(glm::vec3(-25, -.1, -25), glm::vec3(25, .1, 25));
    
    auto skybox = scene->getSkybox();

    GLuint lightVolumeVAO = graphics::CreateVAO();
    GLuint* lightVolumeVBOs = ballModel->meshes[0]->getBuffers();

    glBindBuffer(GL_ARRAY_BUFFER, lightVolumeVBOs[0]);
    glEnableVertexAttribArray(lightVolumeShader["vertex"]);
    glVertexAttribPointer(lightVolumeShader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightVolumeVBOs[3]);

    GLuint gBuffer = graphics::CreateFrameBuffer();
    GLuint gPosition = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
    GLuint gNormal = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA32F);
    GLuint gDiffuse = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA);
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

    Shader lineShader = Shader(rootDirectory + "resources/shaders/line_shader.vert", rootDirectory + "resources/shaders/line_shader.frag");

    float bbData[] = {
        -1, -1, -1,
        1, -1, -1,
        1, -1, -1,
        1, -1, 1,
        1, -1, 1,
        -1, -1, 1,
        -1, -1, 1,
        -1, -1, -1,

        -1, -1, -1,
        -1, 1, -1,
        1, -1, -1,
        1, 1, -1,
        1, -1, 1,
        1, 1, 1,
        -1, -1, 1,
        -1, 1, 1,

        -1, 1, -1,
        1, 1, -1,
        1, 1, -1,
        1, 1, 1,
        1, 1, 1,
        -1, 1, 1,
        -1, 1, 1,
        -1, 1, -1
    };

    GLuint bbVao = graphics::CreateVAO();
    GLuint bbVbo;
    glGenBuffers(1, &bbVbo);
    glBindBuffer(GL_ARRAY_BUFFER, bbVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bbData), bbData, GL_STATIC_DRAW);

    glEnableVertexAttribArray(lineShader["vertex"]);
    glVertexAttribPointer(lineShader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bbVbo);

    Shader computeShader;
    computeShader.AttachShaderFromFile(GL_COMPUTE_SHADER, rootDirectory + "resources/shaders/compute.glsl");
    computeShader.CreateAndLinkProgram();
    std::cout << "compute shader uniforms" << std::endl;
    computeShader.AutoDetectVariables();

    std::cout << "vendor: " << glGetString(GL_VENDOR) << std::endl;
    std::cout << "renderer: " << glGetString(GL_RENDERER) << std::endl;
    std::cout << "version: " << glGetString(GL_VERSION) << std::endl;
    glm::ivec3 work_group_cnt;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &work_group_cnt[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &work_group_cnt[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &work_group_cnt[2]);

    std::cout << "max global work group count: " << work_group_cnt.x << ", " << work_group_cnt.y
        << ", " << work_group_cnt.z << std::endl;

    glm::ivec3 work_group_size;
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &work_group_size[0]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &work_group_size[1]);
    glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &work_group_size[2]);

    std::cout << "max global work group size: " << work_group_size.x << ", " << work_group_size.y
        << ", " << work_group_size.z << std::endl;

    int work_group_invocations;
    glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &work_group_invocations);
    std::cout << "max local work group invocations: " << work_group_invocations << std::endl;


    GLuint computeOutput;
    glGenTextures(1, &computeOutput);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, computeOutput);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Window::getWindowSize().x, Window::getWindowSize().y, 0, GL_RGBA, GL_FLOAT, NULL);
    glBindImageTexture(0, computeOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    // point
    GLuint lightSSBBO;
    glGenBuffers(1, &lightSSBBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 2 * sizeof(glm::vec4) * scene->GetNumPointLights(), NULL, GL_STREAM_COPY);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, lightSSBBO);
    GLint bufMask = GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT;
    glm::vec4* lightBuffer = (glm::vec4*) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, 2 * sizeof(glm::vec4) * scene->GetNumPointLights(), bufMask);
    for( int i = 0; i < scene->GetNumPointLights(); i++) {
        lightBuffer[2 * i + 0] = camera->GetV() * glm::vec4(scene->GetPointLights()[i]->transform.position, 1);
        lightBuffer[2 * i + 1] = glm::vec4(scene->GetPointLights()[i]->intensity * scene->GetPointLights()[i]->color, 1);
    }
    glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );

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

        glm::mat4 VP = camera->GetP() * camera->GetV();
        glm::mat4 V = camera->GetV();
        const auto& pointLights = scene->GetPointLights();
        glm::vec4* lightBuffer = (glm::vec4*) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, 2 * sizeof(glm::vec4) * scene->GetNumPointLights(), bufMask);
        for (int i = 0; i < pointLights.size(); ++i) {
            lightBuffer[2 * i + 0] = camera->GetV() * glm::vec4(pointLights[i]->transform.position, 1);
            lightBuffer[2 * i + 1] = glm::vec4(pointLights[i]->intensity * pointLights[i]->color, 1);
        }
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );

        computeShader.Enable();
        glBindImageTexture(0, computeOutput, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
        glBindImageTexture(1, gPosition, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(2, gNormal, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glBindImageTexture(3, gDiffuse, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA8);
        glBindImageTexture(4, gSpecularExp, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA32F);
        glUniform1i(computeShader["numPointLights"], scene->GetNumPointLights());

        glDispatchCompute(Window::getWindowSize().x / 16, Window::getWindowSize().y / 16, 1);

        // make sure writing to image has finished before read
        // glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        graphics::ToggleDepthBufferWriting(false);
        graphics::ToggleDepthTesting(false);

        combineShader.Enable();
        glBindVertexArray(quadVAO);
        graphics::Bind2DTexture(computeOutput, combineShader["computeOutput"], 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        graphics::ToggleDepthTesting(true);
        graphics::ToggleDepthBufferWriting(true);

        /*

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
            float cutOffIntensity = 0.03;
            float lightRadius = std::sqrtf(light->intensity / cutOffIntensity);
            //lightRadius = 4;
            glm::mat4 lightModelMatrix(1);
            lightModelMatrix = glm::translate(lightModelMatrix, light->transform.position);
            lightModelMatrix = glm::scale(lightModelMatrix, glm::vec3(lightRadius));

            // calculate MVP
            glm::mat4 MVP = VP * lightModelMatrix;

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
        */

        skybox->Render(*camera);

        /*
        Frustum frustum(*camera);

        lineShader.Enable();
        glUniform3fv(lineShader["color"], 1, glm::value_ptr(glm::vec3(0, 1, 0)));
        glBindVertexArray(bbVao);

        glm::mat4 M = planeObj->boundingBox.GetModelMatrix();
        glUniformMatrix4fv(lineShader["MVP"], 1, GL_FALSE, glm::value_ptr(VP * M));
        glDrawArrays(GL_LINES, 0, 24);
        int drawn = 0;

        for (const auto& obj : scene->GetPointLights()) {
            if (frustum.boxInFrustum(obj->boundingBox)) {
                drawn++;
                glm::mat4 M = obj->boundingBox.GetModelMatrix();
                glUniformMatrix4fv(lineShader["MVP"], 1, GL_FALSE, glm::value_ptr(VP * M));
                // glDrawArrays(GL_LINES, 0, 24);
            }
        }
        */
        
        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}