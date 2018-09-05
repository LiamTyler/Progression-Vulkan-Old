#include "progression.h"
// #include "primary_canvas.h"


using namespace Progression;

std::string rootDirectory;

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
    PG::ResourceManager::Init(conf);

    auto screen = PG::Window::getUIScreen();
    screen->setVisible(true);
    screen->performLayout();

    Camera camera = Camera(Transform(
        glm::vec3(0, 0, 5),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 1, 0)));
    camera.AddComponent<UserCameraComponent>(new UserCameraComponent(&camera));
    
    Shader& phongShader = *ResourceManager::GetShader("default-mesh");
    Model* model = ResourceManager::LoadModel("models/single_cube_red_mat.obj");
    // Model* model = ResourceManager::LoadModel("models/cube2.obj");
    if (!model) {
        std::cout << "no model" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    std::cout << "num meshes: " << model->meshMaterialPairs.size() << std::endl;
    
    Mesh* mesh = model->meshMaterialPairs[0].first;
    Material* mat = model->meshMaterialPairs[0].second;
    std::cout << "mat ambient: " << mat->ambient << std::endl;
    std::cout << "mat diffuse: " << mat->diffuse << std::endl;
    std::cout << "mat specular: " << mat->specular << std::endl;
    std::cout << "mat shininess: " << mat->shininess << std::endl;

    GLuint modelVAO;
    glGenVertexArrays(1, &modelVAO);
    glBindVertexArray(modelVAO);
    GLuint* vbos_ = mesh->getBuffers();
    
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glEnableVertexAttribArray(phongShader["vertex"]);
    glVertexAttribPointer(phongShader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbos_[1]);
    glEnableVertexAttribArray(phongShader["normal"]);
    glVertexAttribPointer(phongShader["normal"], 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbos_[2]);
    glEnableVertexAttribArray(phongShader["inTexCoord"]);
    glVertexAttribPointer(phongShader["inTexCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[3]);
    

    glEnable(GL_DEPTH_TEST);
    // Note:: After changing the input mode, should poll for events again
    glfwSetInputMode(Window::getGLFWHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    PG::Input::PollEvents();

    // Game loop
    while (!glfwWindowShouldClose(Window::getGLFWHandle())) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            glfwSetWindowShouldClose(Window::getGLFWHandle(), true);

        float dt = Time::deltaTime();
        camera.Update();

        const auto& bgColor = PG::Window::getBackgroundColor();
        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        //glClearColor(0, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        
        phongShader.Enable();
        glBindVertexArray(modelVAO);

        glm::mat4 P = camera.GetP();
        glm::mat4 V = camera.GetV();
        glUniformMatrix4fv(phongShader["projectionMatrix"], 1, GL_FALSE, glm::value_ptr(P));

        glUniform3fv(phongShader["lightColor"], 1, glm::value_ptr(glm::vec3(1)));
        glm::vec3 lEye = glm::vec3(V * glm::vec4(glm::normalize(glm::vec3(1, -1, -1)), 0));
        glUniform3fv(phongShader["lightInEyeSpace"], 1, glm::value_ptr(lEye));


        glUniform3fv(phongShader["ka"], 1, glm::value_ptr(mat->ambient));
        glUniform3fv(phongShader["kd"], 1, glm::value_ptr(mat->diffuse));
        glUniform3fv(phongShader["ks"], 1, glm::value_ptr(mat->specular));
        glUniform1f(phongShader["specular"], mat->shininess);
        glUniform1i(phongShader["textured"], false);
        glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, modelTex.GetHandle());
        glUniform1i(phongShader["diffuseTex"], 0);
        // send model and normal matrices

        glm::mat4 modelMatrix(1);
        glm::mat4 MV = camera.GetV() * modelMatrix;
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
        glUniformMatrix4fv(phongShader["modelViewMatrix"], 1, GL_FALSE, glm::value_ptr(MV));
        glUniformMatrix4fv(phongShader["normalMatrix"], 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glDrawElements(GL_TRIANGLES, mesh->getNumTriangles() * 3, GL_UNSIGNED_INT, 0);
        

        // Draw UI & primary canvas
        screen->drawContents();
        screen->drawWidgets();

        PG::Window::EndFrame();
    }

    PG::ResourceManager::Free();
    Input::Free();
    Time::Free();
    PG::Window::Free();

    return 0;
}
