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

    Shader shader;
    ShaderFileDesc desc;
    desc.vertex = PG_RESOURCE_DIR "test.vert";
    desc.fragment = PG_RESOURCE_DIR "test.frag";
    if (!loadShaderFromText(shader, desc)) {
        LOG_ERR("Could not load the shader");
        return 1;
    }

    // need this for images that dont have data/rows a multiple of the default (4)
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    TextureUsageDesc texDesc;
    Texture2D tex;
    if (!loadTexture2D(tex, PG_RESOURCE_DIR "textures/chalet.jpg", texDesc, false)) {
        LOG_ERR("Could not load image");
        return 1;
    }

    Model model;
    if (!loadModelFromObj(model, PG_RESOURCE_DIR "models/chalet2.obj", true)) {
        LOG_ERR("Could not load model");
        return 1;
    }

    graphicsApi::toggleDepthTest(true);
    graphicsApi::toggleDepthWrite(true);

    PG::Input::PollEvents();

    Transform modelTransform(
            glm::vec3(0, -1, -5),
            glm::vec3(0, glm::radians(90.0f), glm::radians(90.0f)),
            glm::vec3(1));
    Camera camera(glm::vec3(0), glm::vec3(0));

    glm::vec3 lightDir = -glm::normalize(glm::vec3(.5, -.2, -1));

    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

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
            shader.setUniform("kd", mat.diffuse);
            shader.setUniform("ks", mat.specular);
            shader.setUniform("ke", mat.emissive);
            shader.setUniform("shininess", mat.shininess);
            if (mat.diffuseTexture) {
                shader.setUniform("textured", true);
                graphicsApi::bind2DTexture(mat.diffuseTexture->gpuHandle(), shader.getUniform("diffuseTex"), 0);
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
