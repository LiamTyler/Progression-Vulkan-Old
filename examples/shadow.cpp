#include "progression.hpp"
#include <iomanip>

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

    auto light = scene->GetDirectionalLights()[0];

    
    auto shadowMap = ShadowMap();

    // setup the general data stuff
    float quadVerts[] = {
        -1, 1,
        -1, -1,
        1, -1,

        -1, 1,
        1, -1,
        1, 1
    };
    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glBindVertexArray(quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    /*
    float cubeVerts[] = {
        -1, -1, 1,
        1, -1, 1,
        1, 1, 1,
        -1, 1, 1,
        -1, -1, -1,
        1, -1, -1,
        1, 1, -1,
        -1, 1, -1
    };
    */


    Shader lineShader;
    if (!lineShader.Load(PG_RESOURCE_DIR "shaders/line.vert", PG_RESOURCE_DIR "shaders/line.frag")) {
        LOG_ERR("Could not load line shader");
        exit(EXIT_FAILURE);
    }

    auto cubeModel = ResourceManager::GetModel("cube");
    GLuint* vbos = &cubeModel->meshes[0]->vbos[0];
    GLuint cubeVAO;
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[Mesh::vboName::VERTEX]);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[Mesh::vboName::INDEX]);

    Shader depthWriteShader;
    if (!depthWriteShader.Load(PG_RESOURCE_DIR "shaders/shadow.vert", PG_RESOURCE_DIR "shaders/shadow.frag")) {
        LOG_ERR("Could not load shadow shader");
        exit(EXIT_FAILURE);
    }

    Shader displayShadowShader;
    if (!displayShadowShader.Load(PG_RESOURCE_DIR "shaders/draw_shadow.vert", PG_RESOURCE_DIR "shaders/draw_shadow.frag")) {
        LOG_ERR("Could not load draw shadow shader");
        exit(EXIT_FAILURE);
    }
    
    Shader shadowPhongShader;
    if (!shadowPhongShader.Load(PG_RESOURCE_DIR "shaders/phong_shadow.vert", PG_RESOURCE_DIR "shaders/phong_forward_shadow.frag")) {
        LOG_ERR("Could not load draw shadow shader");
        exit(EXIT_FAILURE);
    }

	Window::SetRelativeMouse(true);
	PG::Input::PollEvents();

	graphics::BindFrameBuffer();
    // glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

	// Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene->Update();
        RenderSystem::UpdateLights(scene, camera);

        // render scene from light's POV
        shadowMap.BindForWriting();

        Frustum frustum = camera->GetFrustum();
        float np = camera->GetNearPlane();
        float fp = camera->GetFarPlane();
        glm::vec3 frustCenter = 0.5f*(fp - np) * camera->GetForwardDir() + camera->transform.position;
        glm::vec3 lightDir = getDirection(light->transform.rotation);
        glm::vec3 lightPos = glm::vec3(0.0f, fp - np, 0.0f);
        // glm::vec3 lightPos = frustCenter;
        // glm::mat4 lightView = glm::lookAt(lightPos, lightPos + lightDir, glm::vec3(0, 1, 0));
        glm::mat4 lightView = glm::lookAt(glm::vec3(0), lightDir, glm::vec3(0, 1, 0));
        // glm::mat4 lightView = glm::lookAt(-10.0f * lightDir, glm::vec3(0), glm::vec3(0, 1, 0));
        //glm::mat4 lightView = camera->GetV();

        glm::vec3 LSCorners[8];
        for (int i = 0; i < 8; ++i) {
            LSCorners[i] = glm::vec3(lightView * glm::vec4(frustum.corners[i], 1));
        }
        BoundingBox lsmBB(LSCorners[0], LSCorners[0]);
        lsmBB.Encompass(LSCorners + 1, 7);

        // glm::mat4 lightProj = glm::ortho(lsmBB.min.x, lsmBB.max.x, lsmBB.min.y, lsmBB.max.y, np, fp);
        // glm::mat4 lightProj = glm::ortho<float>(lsmBB.min.x, lsmBB.max.x, lsmBB.min.y, lsmBB.max.y, -30.0f, 20.0f);
        // glm::mat4 lightProj = glm::ortho<float>(-30, 30, -30, 30, -50, 50);
        
        // glm::vec3 pMin(-140, -100, -100);
        // glm::vec3 pMax(140, 100, 100);
        glm::vec3 pMin(-70, 0, -50);
        glm::vec3 pMax(70, 70, 50);
        glm::mat4 lightProj = glm::ortho<float>(pMin.x, pMax.x, pMin.y, pMax.y, pMin.z, pMax.z);
        // glm::mat4 lightProj = glm::ortho<float>(-140, 140, -100, 100, -100, 100);
        // glm::mat4 lightProj = glm::ortho<float>(-20.0f, 20.0f, -20.0f, 20.0f, -100, 100);

        glm::mat4 lightSpaceMatrix = lightProj * lightView;

        depthWriteShader.Enable();
        auto meshRenderer = RenderSystem::GetSubSystem<MeshRenderSubSystem>();
        meshRenderer->DepthRender(depthWriteShader, lightSpaceMatrix);

        /*
        // Render the shadow map texture, just for debugging purposes
        glViewport(0, 0, Window::width(), Window::height());
	    graphics::BindFrameBuffer();
        displayShadowShader.Enable();
        graphics::SetClearColor(glm::vec4(0));
        graphics::Clear();
        graphics::Bind2DTexture(depthTex, displayShadowShader["tex"], 0);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        */
        

        glViewport(0, 0, Window::width(), Window::height());
        graphics::BindFrameBuffer();
        // glCullFace(GL_BACK);
        shadowPhongShader.Enable();
        graphics::SetClearColor(glm::vec4(1));
        graphics::Clear();

        glUniform3fv(shadowPhongShader["lightDir"], 1, glm::value_ptr(lightDir));
        glUniform3fv(shadowPhongShader["cameraPos"], 1, glm::value_ptr(camera->transform.position));
        glm::mat4 VP = camera->GetP() * camera->GetV();
        glUniformMatrix4fv(shadowPhongShader["VP"], 1, GL_FALSE, glm::value_ptr(VP));
        glUniformMatrix4fv(shadowPhongShader["lightSpaceMatrix"], 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
        graphics::Bind2DTexture(shadowMap.texture(), shadowPhongShader["depthTex"], 1);
        meshRenderer->ShadowRender(scene, shadowPhongShader, *camera);

        // Render the projection bounding box
        lineShader.Enable();
        // glm::mat4 invLSV = glm::inverse(lightView)
        // glm::vec3 wsPMin = glm::vec3(
        glm::mat4 model = glm::inverse(lightSpaceMatrix);
        // model = glm::scale(model, glm::vec3(5));
        glm::mat4 MVP = camera->GetP() * camera->GetV() * model;
        glUniformMatrix4fv(lineShader["MVP"], 1, GL_FALSE, glm::value_ptr(MVP));
        glUniform3fv(lineShader["color"], 1, glm::value_ptr(glm::vec3(0, 1, 0)));
        glBindVertexArray(cubeVAO);
        glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glPolygonMode( GL_FRONT_AND_BACK, GL_FILL);
        

		// RenderSystem::Render(scene);

        PG::Window::EndFrame();
    }

    delete scene;

    PG::EngineQuit();

    return 0;
}
