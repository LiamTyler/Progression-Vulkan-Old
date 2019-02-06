#include "progression.hpp"
#include <iomanip>

#pragma GCC diagnostic ignored "-Wunused-variable"

using namespace Progression;

glm::vec3 getDirection(const glm::vec3& rotation) {
    glm::vec3 dir(0, 0, -1);
    glm::mat4 rot(1);
    rot = glm::rotate(rot, rotation.z, glm::vec3(0, 0, 1));
    rot = glm::rotate(rot, rotation.y, glm::vec3(0, 1, 0));
    rot = glm::rotate(rot, rotation.x, glm::vec3(1, 0, 0));
    return glm::normalize(glm::vec3(rot * glm::vec4(dir, 0)));
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    PG::EngineInitialize();

    const std::string sceneName = "scene1.pgscn";
    // const std::string sceneName = "shadow.pgscn";

    auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/" + sceneName);
    if (!scene) {
        LOG_ERR("Failed to load scene:");
        exit(EXIT_FAILURE);
    }
    auto camera = scene->GetCamera();
    camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));

    if (sceneName == "scene1.pgscn") {
        auto cube = ResourceManager::GetModel("model");
        for (float x = 0; x < 10; x++) {
            for (float z = 0; z < 10; z++) {
                float randHeight = 2 + 4 * (rand() / static_cast<float>(RAND_MAX));
                glm::vec3 pos = glm::vec3(-80 + 20*x, randHeight, -80 + 20*z);
                GameObject* obj = new GameObject(Transform(pos, glm::vec3(0), glm::vec3(1, randHeight, 1)));
                obj->AddComponent<ModelRenderer>(new ModelRenderer(obj, cube.get()));
                scene->AddGameObject(obj);
            }
        }
    }


    GLuint depthCubeMap;
    glGenTextures(1, &depthCubeMap);
    const int SHADOW_WIDTH  = 1024;
    const int SHADOW_HEIGHT = 1024;
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    auto fbo = graphics::CreateFrameBuffer();
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubeMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    graphics::BindFrameBuffer();

    Shader pointShadowShader;
    if (!pointShadowShader.Load(
                PG_RESOURCE_DIR "shaders/pointShadow.vert",
                PG_RESOURCE_DIR "shaders/pointShadow.frag",
                PG_RESOURCE_DIR "shaders/pointShadow.geom"))
    {
        LOG_ERR("Could not load the point shadow shader");
        exit(EXIT_FAILURE);
    }

    Window::SetRelativeMouse(true);
    PG::Input::PollEvents();


    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene->Update();

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        graphics::BindFrameBuffer(fbo);
        graphics::Clear(GL_DEPTH_BUFFER_BIT);
        auto MRS = RenderSystem::GetSubSystem<MeshRenderSubSystem>();
        MRS->DepthPass(

        // RenderSystem::Render(scene);

        PG::Window::EndFrame();
    }

    delete scene;

    PG::EngineQuit();

    return 0;
}
