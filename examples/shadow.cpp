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

    /*
    if (scene->GetNumPointLights() == 0) {
        LOG_ERR("Scene has no point lights");
        exit(EXIT_FAILURE);
    }
    auto light = scene->GetPointLights()[0];

    auto shadowMap = ShadowMap(ShadowMap::Type::CUBE);

    Shader pointShadowShader;
    if (!pointShadowShader.Load(
                PG_RESOURCE_DIR "shaders/pointShadow.vert",
                PG_RESOURCE_DIR "shaders/pointShadow.frag",
                PG_RESOURCE_DIR "shaders/pointShadow.geom"))
    {
        LOG_ERR("Could not load the point shadow shader");
        exit(EXIT_FAILURE);
    }
    pointShadowShader.AddUniform("shadowMatrices");

    Shader phongShader;
    if (!phongShader.Load(
                PG_RESOURCE_DIR "shaders/phong_forward_shadow.vert",
                PG_RESOURCE_DIR "shaders/phong_forward_shadow.frag"))
    {
        LOG_ERR("Could not load the phong shader");
        exit(EXIT_FAILURE);
    }
    */

    Window::SetRelativeMouse(true);
    PG::Input::PollEvents();

    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene->Update();
        // camera->print();
        RenderSystem::Render(scene);

        /*
        auto MRS = RenderSystem::GetSubSystem<MeshRenderSubSystem>();

        float near_plane = .1f;
        float far_plane = light->radius;
        auto lightPos = light->transform.position;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)shadowMap.width() / (float)shadowMap.height(), near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowMap.BindForWriting();
        pointShadowShader.Enable();
        glUniformMatrix4fv(pointShadowShader["shadowMatrices"], 6, GL_FALSE, glm::value_ptr(shadowTransforms[0]));

        //for (unsigned int i = 0; i < 6; ++i)
        //    glUniformMatrix4fv(pointShadowShader["shadowMatrices[" + std::to_string(i) + "]"], 1, GL_FALSE, glm::value_ptr(shadowTransforms[i]));
        glUniform1f(pointShadowShader["far_plane"], light->radius);
        glUniform3fv(pointShadowShader["lightPos"], 1, glm::value_ptr(lightPos));
        MRS->DepthPass(pointShadowShader, glm::mat4(1));
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        glViewport(0, 0, PG::Window::width(), PG::Window::height());
        graphics::BindFrameBuffer(0); // NOTE: This should be the post process FBO for the real rendering system
        graphics::Clear();
        phongShader.Enable();
        glUniform3fv(phongShader["cameraPos"], 1, glm::value_ptr(camera->transform.position));
        glUniform3fv(phongShader["shadowLight.color"], 1, glm::value_ptr(light->color * light->intensity));
        glm::vec3 dir = rotationToDirection(light->transform.rotation);
        glUniform3fv(phongShader["shadowLight.pos"], 1, glm::value_ptr(light->transform.position));
        glUniform1f(phongShader["shadowLight.rSquared"], light->radius * light->radius);
        glUniform1f(phongShader["far_plane"], light->radius);


        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap.texture());
        glUniform1i(phongShader["depthCube"], 1);

        MRS->DepthRender(phongShader, *camera);
        */

        PG::Window::EndFrame();
    }

    delete scene;

    PG::EngineQuit();

    return 0;
}
