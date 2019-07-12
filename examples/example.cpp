#include "progression.hpp"
#include <iomanip>
#include <thread>
#include <future>

#ifdef __linux__ 
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

using namespace Progression;

int main(int argc, char* argv[]) {
    PG_UNUSED(argc);
    PG_UNUSED(argv);

    PG::EngineInitialize();

    Window* window = getMainWindow();
    //window->setRelativeMouse(true);

    ResourceManager::loadResourceFileAsync(PG_RESOURCE_DIR "scenes/resource.txt");
    ResourceManager::waitUntilLoadComplete();
    LOG("Loading fast file");
    ResourceManager::loadFastFile(PG_RESOURCE_DIR "scenes/resource2.txt.ff");

    Model& model = *ResourceManager::get<Model>("chalet");

    // Shader shader;
    // {
    //     Shader s2;
    //     s2.metaData.vertex.filename = PG_RESOURCE_DIR "test.vert";
    //     s2.metaData.fragment.filename = PG_RESOURCE_DIR "test.frag";
    //     s2.load();
    //     GLint len;
    //     GLenum format;
    //     auto binary = s2.getShaderBinary(len, format);
    //     shader.loadFromBinary(binary.data(), len, format);
    //     shader.queryUniforms();
    // }
    Shader& shader = *ResourceManager::get<Shader>("test");

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

    PG_ASSERT(ResourceManager::get<Material>("cockatoo") != nullptr);
    Material& mat = *ResourceManager::get<Material>("cockatoo");

    graphicsApi::toggleDepthTest(true);
    graphicsApi::toggleDepthWrite(true);

    PG::Input::PollEvents();

    Transform modelTransform(
            glm::vec3(0, 0, 0),
            //glm::vec3(0),
            glm::vec3(glm::radians(-90.0f), glm::radians(90.0f), glm::radians(-0.0f)),
            glm::vec3(1));
    Camera camera(glm::vec3(0, 0, 3), glm::vec3(0));

    glm::vec3 lightDir = -glm::normalize(glm::vec3(0, 0, -1));

    // Game loop
    while (!PG::EngineShutdown) {
        window->startFrame();
        PG::Input::PollEvents();

        ResourceManager::update();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC)) {
            PG::EngineShutdown = true;
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

        // graphicsApi::bindVao(quadVao);
        // shader.setUniform("kd", mat.Kd);
        // shader.setUniform("ks", mat.Ks);
        // shader.setUniform("ke", mat.Ke);
        // shader.setUniform("shininess", mat.Ns);
        // if (mat.map_Kd) {
        //     shader.setUniform("textured", true);
        //     graphicsApi::bind2DTexture(mat.map_Kd->gpuHandle(), shader.getUniform("diffuseTex"), 0);
        // } else {
        //     shader.setUniform("textured", false);
        // }
        // glDrawArrays(GL_TRIANGLES, 0, 6);

        for (size_t i = 0; i < model.meshes.size(); ++i) {
            const auto& mesh = model.meshes[i];
            const auto& m = *model.materials[i];
            graphicsApi::bindVao(mesh.vao);
            shader.setUniform("kd", m.Kd);
            shader.setUniform("ks", m.Ks);
            shader.setUniform("ke", m.Ke);
            shader.setUniform("shininess", m.Ns);
            if (m.map_Kd) {
                shader.setUniform("textured", true);
                graphicsApi::bind2DTexture(m.map_Kd->gpuHandle(), shader.getUniform("diffuseTex"), 0);
            } else {
                shader.setUniform("textured", false);
            }

            glDrawElements(GL_TRIANGLES, mesh.getNumIndices(), GL_UNSIGNED_INT, 0);
        }

        window->endFrame();
    }

    PG::EngineQuit();

    return 0;
}
