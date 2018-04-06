#include "include/utils.h"
#include "include/camera.h"
#include "include/shader.h"
#include "include/image.h"
#include "include/fps_counter.h"
#include "include/mesh.h"
#include "include/window.h"
#include "include/user_camera.h"
#include "include/lights.h"

using namespace std;

int main(int arc, char** argv) {
    Window window("Starter Project", 800, 600);

    UserCamera camera = UserCamera(Transform(
                glm::vec3(0, 0, 5),
                glm::vec3(0, 0, -1),
                glm::vec3(0, 1, 0)));
    Shader shader(
            "Phong Shader",
            "shaders/regular_phong.vert",
            "shaders/regular_phong.frag");
    Mesh mesh("models/key.obj");
    GLuint vao;
    GLuint vbo[3];
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(3, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, mesh.numVertices * sizeof(glm::vec3), mesh.vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(shader["vertex"]);
    glVertexAttribPointer(shader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, mesh.numVertices * sizeof(glm::vec3), mesh.normals, GL_STATIC_DRAW);
    glEnableVertexAttribArray(shader["normal"]);
    glVertexAttribPointer(shader["normal"], 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.numTriangles * sizeof(glm::ivec3),
        mesh.indices, GL_STATIC_DRAW);

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

        shader.Enable();
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 V = camera.GetV();
        glm::mat4 P = camera.GetP();
        glUniformMatrix4fv(shader["projectionMatrix"], 1,  GL_FALSE, glm::value_ptr(P));

        // light
        DirectionalLight dl(glm::vec3(0, 0, 1));
        glUniform3fv(shader["Ia"], 1, glm::value_ptr(dl.Ia));
        glUniform3fv(shader["Id"], 1, glm::value_ptr(dl.Id));
        glUniform3fv(shader["Is"], 1, glm::value_ptr(dl.Is));
        glm::vec3 lEye = glm::vec3(V * glm::vec4(dl.direction, 0));
        glUniform3fv(shader["lightInEyeSpace"], 1, glm::value_ptr(lEye));

		// draw model
        glm::mat4 model(1);
		glm::mat4 MV = V * model;
		glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
		glUniformMatrix4fv(shader["modelViewMatrix"], 1, GL_FALSE, value_ptr(MV));
		glUniformMatrix4fv(shader["normalMatrix"], 1, GL_FALSE, value_ptr(normalMatrix));

        // hard code material for now
        glm::vec3 ka(1, .4, .4);
        glm::vec3 kd(1, .4, .4);
        glm::vec3 ks(.6, .6, .6);
        float spec = 50;
		glUniform3fv(shader["ka"], 1, value_ptr(ka));
		glUniform3fv(shader["kd"], 1, value_ptr(kd));
		glUniform3fv(shader["ks"], 1, value_ptr(ks));
		glUniform1f(shader["specular"], spec);

        glDrawElements(GL_TRIANGLES, mesh.numTriangles*3, GL_UNSIGNED_INT, 0);

        window.EndFrame();
    }

    return 0;
}
