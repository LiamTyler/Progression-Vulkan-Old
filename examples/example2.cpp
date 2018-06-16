#include "starter.h"

using namespace std;

int main(int arc, char** argv) {
    Window window("OpenGL_Starter Example 2", 800, 600);

    UserCamera camera = UserCamera(Transform(
                glm::vec3(0, 0, 5),
                glm::vec3(0, 0, -1),
                glm::vec3(0, 1, 0)));
    Shader shader(
            "Phong Shader",
            "../../shaders/regular_phong.vert",
            "../../shaders/regular_phong.frag");

    DirectionalLight light(
            glm::vec3(0, -1, -1),
            glm::vec3(0.0, 0.0, 0.0),
            glm::vec3(0.7, 0.7, 0.7),
            glm::vec3(1.0, 1.0, 1.0));

    Model model("../../models/piano2.obj");
    // Model model("models/cube.obj");
    // Model model("models/test.obj");
    model.Load();

    GameObject gameObj;
    gameObj.AddComponent<ModelRenderer>(new ModelRenderer(&shader, &model));

    window.SetRelativeMouse(true);
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        window.StartFrame();
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                        camera.velocity.z = 1;
                        break;
                    case SDLK_s:
                        camera.velocity.z = -1;
                        break;
                    case SDLK_a:
                        camera.velocity.x = -1;
                        break;
                    case SDLK_d:
                        camera.velocity.x = 1;
                        break;
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                    case SDLK_p:
                        break;
                    case SDLK_SPACE:
                        break;
                }
            } else if (event.type == SDL_KEYUP) {
                switch (event.key.keysym.sym) {
                    case SDLK_w:
                    case SDLK_s:
                        camera.velocity.z = 0;
                        break;
                    case SDLK_a:
                    case SDLK_d:
                        camera.velocity.x = 0;
                        break;
                    case SDLK_SPACE:
                        break;
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                float dx = -event.motion.xrel;
                float dy = -event.motion.yrel;
                camera.Rotate(glm::vec3(dy, dx, 0));
            }
        }

        float dt = window.GetDT();
        camera.Update(dt);
        gameObj.Update(dt);

        shader.Enable();
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 V = camera.GetV();
        glm::mat4 P = camera.GetP();
        glUniformMatrix4fv(shader["projectionMatrix"], 1,  GL_FALSE, glm::value_ptr(P));

        // light
        glUniform3fv(shader["Ia"], 1, glm::value_ptr(light.Ia));
        glUniform3fv(shader["Id"], 1, glm::value_ptr(light.Id));
        glUniform3fv(shader["Is"], 1, glm::value_ptr(light.Is));
        glm::vec3 lEye = glm::vec3(V * glm::vec4(light.direction, 0));
        glUniform3fv(shader["lightInEyeSpace"], 1, glm::value_ptr(lEye));

        gameObj.GetComponent<ModelRenderer>()->Render(camera);

        window.EndFrame();
    }

    return 0;
}
