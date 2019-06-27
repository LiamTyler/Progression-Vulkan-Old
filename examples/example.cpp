#include "progression.hpp"
#include <iomanip>
#include <thread>

#ifdef __linux__ 
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

using namespace Progression;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    PG::EngineInitialize();

    Window* window = getMainWindow();
    window->setRelativeMouse(true);

    ResourceManager::init();

    // if (!Resource::loadResourceFile(PG_RESOURCE_DIR "resource.txt")) {
    //     LOG_ERR("Could not load the resource file");
    //     return 1;
    // }


    // Shader& shader = *ResourceManager::get<Shader>("test");
    // Model& model   = *ResourceManager::get<Model>("models/chalet2.obj");

    ResourceManager::loadResourceFileAsync(PG_RESOURCE_DIR "scenes/resource.txt");
    LOG("About to load 2");
    ResourceManager::loadResourceFileAsync(PG_RESOURCE_DIR "scenes/resource2.txt");
    LOG("About to wait");
    ResourceManager::waitUntilLoadComplete();
    LOG("Waiting done");

    ShaderMetaData smd;
    smd.vertex   = TimeStampedFile(PG_RESOURCE_DIR "test.vert");
    smd.fragment = TimeStampedFile(PG_RESOURCE_DIR "test.frag");
    //Shader& shader = *ResourceManager::loadShader("test", smd);
    Shader& shader = *ResourceManager::get<Shader>("test");
    PG_ASSERT(ResourceManager::get<Shader>("flat"));

    float quadVerts[] = {
        -1, 1, 0,   0, 0, 1,    0, 1,
        -1, -1,0,   0, 0, 1,    0, 0,
        1, -1, 0,   0, 0, 1,    1, 0,

        -1, 1, 0,    0, 0, 1,    0, 1,
        1, -1, 0,    0, 0, 1,    1, 0,
        1, 1,  0,    0, 0, 1,    1, 1,
    };

    auto quadVao = graphicsApi::createVao();
    auto quadVbo = graphicsApi::createBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    graphicsApi::describeAttribute(0, 3, GL_FLOAT, sizeof(GL_FLOAT) * 8, sizeof(float) * 0);
    graphicsApi::describeAttribute(1, 3, GL_FLOAT, sizeof(GL_FLOAT) * 8, sizeof(float) * 3);
    graphicsApi::describeAttribute(2, 2, GL_FLOAT, sizeof(GL_FLOAT) * 8, sizeof(float) * 6);

    Material mat;
    mat.Ka = glm::vec3(0);
    mat.Kd = glm::vec3(1, 0, 0);
    mat.Ks = glm::vec3(0);
    mat.Ke = glm::vec3(0);
    mat.Ns = 50;
    mat.map_Kd = nullptr;

    // TextureMetaData tmd;
    // tmd.file.filename = PG_RESOURCE_DIR "textures/cockatoo.jpg";
    // mat.map_Kd = ResourceManager::loadTexture2D("cockatoo", tmd);


    graphicsApi::toggleDepthTest(true);
    graphicsApi::toggleDepthWrite(true);

    PG::Input::PollEvents();

    Transform modelTransform(
            glm::vec3(0, 0, 0),
            glm::vec3(0, 0, 0),
            glm::vec3(1));
    Camera camera(glm::vec3(0, 0, 5), glm::vec3(0));

    glm::vec3 lightDir = -glm::normalize(glm::vec3(0, 0, -1));

    // Game loop
    while (!PG::EngineShutdown) {
        window->startFrame();
        PG::Input::PollEvents();

        ResourceManager::update();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;
        if (PG::Input::GetKeyDown(PG::PG_K_U)) {
            LOG("updating...");
            //ResourceManager::join();
            LOG("done");
        }
        if (PG::Input::GetKeyDown(PG::PG_K_R)) {
            ResourceManager::shutdown();
            exit(EXIT_SUCCESS);
        }

        graphicsApi::clearColor(0, 0, 0, 0);
        graphicsApi::clearColorBuffer();
        graphicsApi::clearDepthBuffer();
        shader.enable();
        glm::mat4 M = modelTransform.getModelMatrix();
        glm::mat4 N = glm::transpose(glm::inverse(M));
        glm::mat4 MVP = camera.getVP() * M;
        shader.setUniform("M", M);
        shader.setUniform("N", N);
        shader.setUniform("MVP", MVP);

        shader.setUniform("cameraPos", camera.position);
        shader.setUniform("lightDirInWorldSpace", lightDir);

        graphicsApi::bindVao(quadVao);
        shader.setUniform("kd", mat.Kd);
        shader.setUniform("ks", mat.Ks);
        shader.setUniform("ke", mat.Ke);
        shader.setUniform("shininess", mat.Ns);
        if (mat.map_Kd) {
            shader.setUniform("textured", true);
            graphicsApi::bind2DTexture(mat.map_Kd->gpuHandle(), shader.getUniform("diffuseTex"), 0);
        } else {
            shader.setUniform("textured", false);
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //     glDrawElements(GL_TRIANGLES, mesh.getNumIndices(), GL_UNSIGNED_INT, 0);
        // for (size_t i = 0; i < model.meshes.size(); ++i) {
        //     const auto& mesh = model.meshes[i];
        //     const auto& mat = *model.materials[i];
        //     graphicsApi::bindVao(mesh.vao);
        //     shader.setUniform("kd", mat.Kd);
        //     shader.setUniform("ks", mat.Ks);
        //     shader.setUniform("ke", mat.Ke);
        //     shader.setUniform("shininess", mat.Ns);
        //     if (mat.map_Kd) {
        //         shader.setUniform("textured", true);
        //         graphicsApi::bind2DTexture(mat.map_Kd->gpuHandle(), shader.getUniform("diffuseTex"), 0);
        //     } else {
        //         shader.setUniform("textured", false);
        //     }

        //     glDrawElements(GL_TRIANGLES, mesh.getNumIndices(), GL_UNSIGNED_INT, 0);
        // }

        window->endFrame();
    }


    PG::EngineQuit();

    return 0;
}
