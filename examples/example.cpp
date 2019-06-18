#include "progression.hpp"
#include <iomanip>

#ifdef __linux__ 
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

using namespace Progression;

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    PG::EngineInitialize();

    Window::SetRelativeMouse(true);

    if (!Resource::loadResourceFile(PG_RESOURCE_DIR "resource.txt")) {
        LOG_ERR("Could not load the resource file");
        return 1;
    }


    Shader& shader = *Resource::getShader("test");
    Model& model   = *Resource::getModel("models/chalet2.obj");

    graphicsApi::toggleDepthTest(true);
    graphicsApi::toggleDepthWrite(true);

    PG::Input::PollEvents();

    Transform modelTransform(
            glm::vec3(0, -0.5, -2),
            glm::vec3(glm::radians(-90.0f), glm::radians(90.0f), glm::radians(0.0f)),
            glm::vec3(1));
    Camera camera(glm::vec3(0), glm::vec3(0));

    glm::vec3 lightDir = -glm::normalize(glm::vec3(.5, -.2, -1));

    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        Resource::update();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;
        if (PG::Input::GetKeyDown(PG::PG_K_U)) {
            LOG("updating...");
            Resource::join();
            LOG("done");
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

        for (size_t i = 0; i < model.meshes.size(); ++i) {
            const auto& mesh = model.meshes[i];
            const auto& mat = *model.materials[i];
            graphicsApi::bindVao(mesh.vao);
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

            glDrawElements(GL_TRIANGLES, mesh.getNumIndices(), GL_UNSIGNED_INT, 0);
        }

        PG::Window::EndFrame();
    }


    PG::EngineQuit();

    return 0;
}
