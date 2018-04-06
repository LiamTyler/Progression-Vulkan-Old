#include "include/utils.h"
#include "include/camera.h"
#include "include/shader.h"
#include "include/image.h"
#include "include/fps_counter.h"
#include "include/mesh.h"
#include "include/window.h"

using namespace std;

int main(int arc, char** argv) {
    // initialize SDL and GLEW and set up window
    Window window("Starter Project", 800, 600);

    // set up the particle system
    Camera camera = Camera(Transform(
                glm::vec3(0, 0, 5),
                glm::vec3(0, 0, -1),
                glm::vec3(0, 1, 0)));
    Shader shader(
            "Plain Shader",
            "shaders/plain_shader.vert",
            "shaders/plain_shader.frag");
    /*
    static const GLfloat quad_verts[] = {
        -.5, .5, 0,
        -.5, -.5, 0,
        .5, -.5, 0,
        .5, -.5, 0,
        .5, .5, 0,
        -.5, .5, 0,
    };
    GLuint vao;
    GLuint vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad_verts), quad_verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(shader["pos"]);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(shader["pos"], 3, GL_FLOAT, GL_FALSE, 0, 0);
    */
    Mesh mesh("models/key.obj");
    GLuint vao;
    GLuint vbo[3];
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(3, vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, mesh.numVertices * sizeof(glm::vec3), mesh.vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(shader["pos"]);
    glVertexAttribPointer(shader["pos"], 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, mesh.numVertices * sizeof(glm::vec3), mesh.normals, GL_STATIC_DRAW);
    // glEnableVertexAttribArray(shader["norm"]);
    // glVertexAttribPointer(shader["norm"], 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.numTriangles * sizeof(glm::ivec3),
        mesh.indices, GL_STATIC_DRAW);


    window.SetRelativeMouse(true);
    bool quit = false;
    SDL_Event event;
    while (!quit) {
        window.StartFrame();
        // Process all input events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            } else if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
                // key down events (wont repeat if holding key down)
                switch (event.key.keysym.sym) {
                    /*
                    case SDLK_w:
                        camera.VelZ(1.0f);
                        break;
                    case SDLK_s:
                        camera.VelZ(-1.0f);
                        break;
                    case SDLK_a:
                        camera.VelX(-1.0f);
                        break;
                    case SDLK_d:
                        camera.VelX(1.0f);
                        break;
                    */
                    case SDLK_ESCAPE:
                        quit = true;
                        break;
                    case SDLK_p:
                        break;
                    case SDLK_SPACE:
                        break;
                }
            } else if (event.type == SDL_KEYUP) {
                // handle key up events
                switch (event.key.keysym.sym) {
                    /*
                    case SDLK_w:
                    case SDLK_s:
                        camera.VelZ(0.0f);
                        break;
                    case SDLK_a:
                    case SDLK_d:
                        camera.VelX(0.0f);
                        break;
                    */
                    case SDLK_SPACE:
                        break;
                }
            } else if (event.type == SDL_MOUSEMOTION) {
                // handle mouse events
                float dx = event.motion.xrel;
                float dy = event.motion.yrel;
                /*
                camera.RotateX(-dy);
                camera.RotateY(-dx);
                camera.UpdateAxis();
                */
            }
        }


        float dt = window.GetDT();
        // camera.Update(dt);

        // shader.Enable();
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model(1);
        glm::mat4 v = camera.GetV();
        glm::mat4 p = camera.GetP();
        glUniformMatrix4fv(shader["model"], 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(shader["view"], 1,  GL_FALSE, glm::value_ptr(v));
        glUniformMatrix4fv(shader["proj"], 1,  GL_FALSE, glm::value_ptr(p));

        // glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, mesh.numTriangles*3, GL_UNSIGNED_INT, 0);

        window.EndFrame();
    }

    return 0;
}
