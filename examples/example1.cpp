#include "progression.h"
// #include "primary_canvas.h"

using namespace Progression;

std::string rootDirectory;
// PrimaryCanvas* primaryCanvas;

// argv[1] = path of the root directory
int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Please pass in the path of the root directory as the first argument" << std::endl;
        return 0;
    }
    rootDirectory = argv[1];

    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::Window::Init(conf);
    PG::Time::Init(conf);
    Input::Init(conf);

    auto screen = PG::Window::getUIScreen();
    // PrimaryCanvas* primaryCanvas = new PrimaryCanvas(screen);

    screen->setVisible(true);
    screen->performLayout();

    Shader phongShader(
        "phong Shader",
        rootDirectory + "resources/shaders/regular_phong.vert",
        rootDirectory + "resources/shaders/regular_phong.frag");

    Shader deferredOutput(
        "deferredOutput Shader",
        rootDirectory + "resources/shaders/deferred_output.vert",
        rootDirectory + "resources/shaders/deferred_output.frag");

    Shader deferredShader(
        "deferred Shader",
        rootDirectory + "resources/shaders/deferred.vert",
        rootDirectory + "resources/shaders/deferred.frag");


    float quad_verts[] = {
        -1, 1,
        -1, -1,
        1, -1,

        -1, 1,
        1, -1,
        1, 1
    };
    GLuint quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts),
        quad_verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(deferredShader["vertex"]);
    glVertexAttribPointer(deferredShader["vertex"], 2, GL_FLOAT, GL_FALSE, 0, 0);


    Material mat(
        glm::vec3(0),
        glm::vec3(1, 0, 0),
        glm::vec3(.3),
        50);
    DirectionalLight light(
        glm::vec3(-.2, -.8, -1),
        glm::vec3(0.0, 0.0, 0.0),
        glm::vec3(0.7, 0.7, 0.7),
        glm::vec3(1.0, 1.0, 1.0));

    UserCamera camera = UserCamera(Transform(
        glm::vec3(0, 0, 5),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 1, 0)));

    Model cube(rootDirectory + "resources/models/cube.obj");
    cube.Load();
    const auto& meshes = cube.GetMeshes();
    std::cout << "model has " << meshes.size() << " meshes" << std::endl;
    meshes[0]->material = &mat;

    GameObject gameObj;
    gameObj.AddComponent<ModelRenderer>(new ModelRenderer(&deferredOutput, &cube));
    // gameObj.AddComponent<ModelRenderer>(new ModelRenderer(&phongShader, &cube));

    int SW = Window::getWindowSize().x;
    int SH = Window::getWindowSize().y;
    glViewport(0, 0, SW, SH);

    unsigned int gBuffer;
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    unsigned int gPosition, gNormal, gAlbedoSpec;
    // position color buffer
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SW, SH, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
    // normal color buffer
    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SW, SH, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    // color + specular color buffer
    glGenTextures(1, &gAlbedoSpec);
    glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SW, SH, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
    unsigned int attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SW, SH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(Window::getGLFWHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Game loop
    while (!glfwWindowShouldClose(Window::getGLFWHandle())) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (Input::GetKeyDown(PG_K_W)) {
            camera.velocity.z = 1;
        }
        else if (Input::GetKeyDown(PG_K_S)) {
            camera.velocity.z = -1;
        }
        else if (Input::GetKeyDown(PG_K_D)) {
            camera.velocity.x = 1;
        }
        else if (Input::GetKeyDown(PG_K_A)) {
            camera.velocity.x = -1;
        }
        else if (Input::GetKeyDown(PG_K_ESC)) {
            glfwSetWindowShouldClose(Window::getGLFWHandle(), true);
        }

        if (Input::GetKeyUp(PG_K_W)) {
            camera.velocity.z = 0;
        }
        else if (Input::GetKeyUp(PG_K_S)) {
            camera.velocity.z = 0;
        }
        else if (Input::GetKeyUp(PG_K_D)) {
            camera.velocity.x = 0;
        }
        else if (Input::GetKeyUp(PG_K_A)) {
            camera.velocity.x = 0;
        }


        glm::ivec2 dMouse = -Input::GetMouseChange();
        // camera.Rotate(glm::vec3(dMouse.y, dMouse.x, 0));

        float dt = Time::deltaTime();
        //camera.Update(dt);

        glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
        deferredOutput.Enable();
        // phongShader.Enable();

        glClearColor(0, 0, 0, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 P = camera.GetP();
        glm::mat4 V = camera.GetV();
        glUniformMatrix4fv(deferredOutput["projectionMatrix"], 1, GL_FALSE, glm::value_ptr(P));

        /*glUniformMatrix4fv(phongShader["projectionMatrix"], 1, GL_FALSE, glm::value_ptr(P));
        glUniform3fv(phongShader["Ia"], 1, glm::value_ptr(light.Ia));
        glUniform3fv(phongShader["Id"], 1, glm::value_ptr(light.Id));
        glUniform3fv(phongShader["Is"], 1, glm::value_ptr(light.Is));
        glm::vec3 lEye = glm::vec3(V * glm::vec4(light.direction, 0));
        glUniform3fv(phongShader["lightInEyeSpace"], 1, glm::value_ptr(lEye));*/
        
        
        gameObj.GetComponent<ModelRenderer>()->Render(camera);


        // Draw UI & primary canvas
        //screen->drawContents();
        //screen->drawWidgets();

        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        /*
        if (true) {
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            unsigned char* pData = new unsigned char[SW*SH * 4];
            glGetTexImage(GL_TEXTURE_2D, gAlbedoSpec, GL_RGBA, GL_UNSIGNED_BYTE, pData);
            stbi_write_png("test.png", SW, SH, 4, pData, 4 * SW);
            delete[] pData;
        }
        */


        // Now do the deferred shading by combining the textures
        deferredShader.Enable();

        const auto& bgColor = PG::Window::getBackgroundColor();
        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUniform3fv(deferredShader["lightColor"], 1, glm::value_ptr(light.Id));
        glm::vec3 lEye = glm::vec3(V * glm::vec4(light.direction, 0));
        glUniform3fv(deferredShader["lightInEyeSpace"], 1, glm::value_ptr(lEye));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gPosition);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, gNormal);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        

        PG::Window::EndFrame();
    }

    Input::Free();
    Time::Free();
    PG::Window::Free();

    return 0;
}
