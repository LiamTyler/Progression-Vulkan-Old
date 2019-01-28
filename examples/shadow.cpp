#include "progression.hpp"
#include <iomanip>

using namespace Progression;

glm::vec3 getDirection(const glm::vec3& rotation) {
    glm::vec3 dir(0, 0, -1);
    glm::mat4 rot(1);
    rot = glm::rotate(rot, rotation.z, glm::vec3(0, 0, 1));
    rot = glm::rotate(rot, rotation.y, glm::vec3(0, 1, 0));
    rot = glm::rotate(rot, rotation.x, glm::vec3(1, 0, 0));
    return glm::vec3(rot * glm::vec4(dir, 0));
}

int main(int argc, char* argv[]) {
	auto conf = PG::config::Config(PG_ROOT_DIR "configs/default.toml");
	if (!conf) {
        LOG_ERR("Failed to load the config file");
        exit(EXIT_FAILURE);
    }

	PG::EngineInitialize(conf);

	auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/scene1.pgscn");
    if (!scene) {
        LOG_ERR("Failed to load scene:");
        exit(EXIT_FAILURE);
    }
	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));

    auto light = scene->GetDirectionalLights()[0];

    auto depthFBO = graphics::CreateFrameBuffer();

    GLuint depthTex;
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, Window::width(), Window::height(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    Shader depthWriteShader;
    if (!depthWriteShader.Load(PG_RESOURCE_DIR "shaders/shadow.vert", PG_RESOURCE_DIR "shaders/shadow.frag")) {
        LOG_ERR("Could not load shadow shader");
        exit(EXIT_FAILURE);
    }

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

    Shader displayShadowShader;
    if (!displayShadowShader.Load(PG_RESOURCE_DIR "shaders/draw_shadow.vert", PG_RESOURCE_DIR "shaders/draw_shadow.frag")) {
        LOG_ERR("Could not load draw shadow shader");
        exit(EXIT_FAILURE);
    }

	Window::SetRelativeMouse(true);
	PG::Input::PollEvents();

	graphics::BindFrameBuffer();
	// Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene->Update();

        // render scene from light's POV
	    graphics::BindFrameBuffer(depthFBO);
        graphics::Clear(GL_DEPTH_BUFFER_BIT);
        glm::vec3 lightDir = getDirection(light->transform.rotation);
        glm::mat4 lightView = glm::lookAt(light->transform.position, lightDir, light->transform.position + lightDir);
        glm::mat4 lightProj = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, camera->GetNearPlane(), camera->GetFarPlane());

        glm::mat4 lightSpaceMatrix = lightProj * lightView;

        depthWriteShader.Enable();
        auto meshRenderer = RenderSystem::GetSubSystem<MeshRenderSubSystem>();
        meshRenderer->DepthRender(depthWriteShader, lightSpaceMatrix);

        // Render the scene normally, but with the shadow map texture
	    graphics::BindFrameBuffer();
        displayShadowShader.Enable();
        graphics::Bind2DTexture(depthTex, displayShadowShader["tex"]);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

		// RenderSystem::Render(scene);

        PG::Window::EndFrame();
    }

    delete scene;

    PG::EngineQuit();

    return 0;
}
