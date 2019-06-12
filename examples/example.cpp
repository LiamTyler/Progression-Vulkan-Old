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

    ShaderFileDesc desc;
    desc.vertex = PG_RESOURCE_DIR "test.vert";
    desc.fragment = PG_RESOURCE_DIR "test.frag";
    Shader shader;
    if (!loadShaderFromText(shader, desc)) {
        LOG_ERR("Could not load the shader");
        return 0;
    }

    float quadVerts[] = {
        -1, 1,  0, 0, 1, 0, 1,
        -1, -1, 0, 0, 1, 0, 0,
        1, -1,  0, 0, 1, 1, 0,

        -1, 1, 0, 0, 1, 0, 1,
        1, -1, 0, 0, 1, 1, 0,
        1, 1,  0, 0, 1, 1, 1,
    };

    auto quadVao = graphicsApi::createVao();
    auto quadVbo = graphicsApi::createBuffer();
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
    graphicsApi::describeAttribute(0, 2, GL_FLOAT, sizeof(GL_FLOAT) * 7, sizeof(float) * 0);
    graphicsApi::describeAttribute(1, 3, GL_FLOAT, sizeof(GL_FLOAT) * 7, sizeof(float) * 2);
    graphicsApi::describeAttribute(2, 2, GL_FLOAT, sizeof(GL_FLOAT) * 7, sizeof(float) * 5);

    graphicsApi::toggleDepthTest(true);
    graphicsApi::toggleDepthWrite(true);

    PG::Input::PollEvents();

    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        shader.enable();
        
        graphicsApi::bindVao(quadVao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        PG::Window::EndFrame();
    }


    PG::EngineQuit();

    return 0;
}
