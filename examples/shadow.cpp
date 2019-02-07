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

    if (scene->GetNumPointLights() == 0) {
        LOG_ERR("Scene has no point lights");
        exit(EXIT_FAILURE);
    }
    auto light = scene->GetPointLights()[0];


    Window::SetRelativeMouse(true);
    PG::Input::PollEvents();

    GLenum err;
    while((err = glGetError()) != GL_NO_ERROR)
    {
        LOG_ERR("opengl error!")
      // Process/log the error.
    }


    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene->Update();
        RenderSystem::Render(scene);
    while((err = glGetError()) != GL_NO_ERROR)
    {
        LOG_ERR("opengl error!")
      // Process/log the error.
    }
        // auto MRS = RenderSystem::GetSubSystem<MeshRenderSubSystem>();

        // glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        // graphics::BindFrameBuffer(fbo);
        // graphics::Clear(GL_DEPTH_BUFFER_BIT);
        // pointShadowShader.Enable();

        // glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), SHADOW_WIDTH / (float)SHADOW_HEIGHT, 0.1f, light->radius);
        // auto pos = light->transform.position;
        // std::vector<glm::mat4> LSMs = {
        //     shadowProj * glm::lookAt(pos, pos + glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),
        //     shadowProj * glm::lookAt(pos, pos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),
        //     shadowProj * glm::lookAt(pos, pos + glm::vec3(0, 1, 0), glm::vec3(0, 0, -1)),
        //     shadowProj * glm::lookAt(pos, pos + glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),
        //     shadowProj * glm::lookAt(pos, pos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),
        //     shadowProj * glm::lookAt(pos, pos + glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)),
        // };
        //     
        // glUniform1f(pointShadowShader["far_plane"], light->radius);
        // glUniform3fv(pointShadowShader["light_pos"], 1, glm::value_ptr(pos));
        // glUniformMatrix4fv(pointShadowShader["shadowMatrices"], 6, GL_FALSE, glm::value_ptr(LSMs[0]));
        // MRS->DepthPass(pointShadowShader, glm::mat4(1));

        /*
        glViewport(0, 0, PG::Window::width(), PG::Window::height());
        graphics::BindFrameBuffer(0); // NOTE: This should be the post process FBO for the real rendering system
        graphics::Clear();
        phongShader.Enable();
        // glUniform3fv(phongShader["cameraPos"], 1, glm::value_ptr(camera->transform.position));
        // glUniform3fv(phongShader["shadowLight.color"], 1, glm::value_ptr(light->color * light->intensity));
        // glm::vec3 dir = rotationToDirection(light->transform.rotation);
        // glUniform3fv(phongShader["shadowLight.pos"], 1, glm::value_ptr(light->transform.position));
        // glUniform1f(phongShader["shadowLight.rSquared"], light->radius * light->radius);

        // glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubeMap);
        // glUniform1i(phongShader["depthCube"], 0);

        MRS->DepthRender(phongShader, *camera);
        */

        PG::Window::EndFrame();
    }

    delete scene;

    PG::EngineQuit();

    return 0;
}
