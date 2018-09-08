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

    PG::EngineInitialize(conf);

    Scene scene;

    Camera camera = Camera(Transform(
        glm::vec3(0, 0, 5),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 1, 0)));
    camera.AddComponent<UserCameraComponent>(new UserCameraComponent(&camera));

    //Light directionalLight(Light::Type::DIRECTIONAL);
    //directionalLight.transform.rotation = glm::vec3(-glm::radians(45.0f), glm::radians(45.0f), 0);
    //scene.AddLight(&directionalLight);

    Light pointLight1(Light::Type::POINT);
    pointLight1.transform.position = glm::vec3(0, 0, 5);
    pointLight1.intensity = 5;
    scene.AddLight(&pointLight1);

    scene.AddCamera(&camera);
    
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
    

    // Note:: After changing the input mode, should poll for events again
    glfwSetInputMode(Window::getGLFWHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    PG::Input::PollEvents();

    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        camera.Update();

        RenderSystem::Render(&scene);

        phongShader.Enable();
        glBindVertexArray(modelVAO);


        RenderSystem::UploadCameraProjection(phongShader, camera);
        RenderSystem::UploadLights(phongShader);
        RenderSystem::UploadMaterial(phongShader, *mat);

        glm::mat4 modelMatrix(1);
        glm::mat4 MV = camera.GetV() * modelMatrix;
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
        glUniformMatrix4fv(phongShader["modelViewMatrix"], 1, GL_FALSE, glm::value_ptr(MV));
        glUniformMatrix4fv(phongShader["normalMatrix"], 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glDrawElements(GL_TRIANGLES, mesh->getNumTriangles() * 3, GL_UNSIGNED_INT, 0);

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}
