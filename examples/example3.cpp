#include "progression.h"

using namespace std;
using namespace PG; // PG is a shortcut for Progression defined in progression.h

int main(int arc, char** argv) {
    Window window("Starter Project", 800, 600);

    UserCamera camera = UserCamera(Transform(
                glm::vec3(0, 0, 5),
                glm::vec3(0, 0, -1),
                glm::vec3(0, 1, 0)));
    Shader phongShader(
            "Phong Shader",
            "../../shaders/regular_phong.vert",
            "../../shaders/regular_phong.frag");

    DirectionalLight light(
            glm::vec3(0, -1, -1),
            glm::vec3(0.0, 0.0, 0.0),
            glm::vec3(0.7, 0.7, 0.7),
            glm::vec3(1.0, 1.0, 1.0));

    Model model("../../models/piano2.obj");
    model.Load();

    
    vector<string> faces = {
        "../../textures/skybox/water/right.jpg",
        "../../textures/skybox/water/left.jpg",
        "../../textures/skybox/water/top.jpg",
        "../../textures/skybox/water/bottom.jpg",
        "../../textures/skybox/water/front.jpg",
        "../../textures/skybox/water/back.jpg"
    };
    
	Skybox skybox("../../shaders/skybox.vert", "../../shaders/skybox.frag");
	skybox.Load(faces);

    Background background(glm::vec4(1,1,1,1), &skybox);

    GameObject gameObj;
    gameObj.AddComponent<ModelRenderer>(new ModelRenderer(&phongShader, &model));

    window.SetRelativeMouse(true);
    bool quit = false;
    while (!quit) {
        window.StartFrame();

        if (Input::GetKeyDown(PG_K_W)) {
            camera.velocity.z = 1;
        }
        else if (Input::GetKeyDown(PG_K_S)) {
            camera.velocity.z = -1;
        }
        else if (Input::GetKeyDown(PG_K_D)) {
            camera.velocity.x = 1;
        }
        else if (Input::GetKeyDown(PG_K_A)) {
            camera.velocity.x = -1;
        }
        else if (Input::GetKeyDown(PG_K_ESC)) {
            quit = true;
        }

        if (Input::GetKeyUp(PG_K_W)) {
            camera.velocity.z = 0;
        }
        else if (Input::GetKeyUp(PG_K_S)) {
            camera.velocity.z = 0;
        }
        else if (Input::GetKeyUp(PG_K_D)) {
            camera.velocity.x = 0;
        }
        else if (Input::GetKeyUp(PG_K_A)) {
            camera.velocity.x = 0;
        }

        glm::ivec2 dMouse = -Input::GetMouseChange();
        camera.Rotate(glm::vec3(dMouse.y, dMouse.x, 0));

        float dt = window.GetDT();
        camera.Update(dt);
        gameObj.Update(dt);

        background.ClearAndRender(camera);

        phongShader.Enable();
        glm::mat4 P = camera.GetP();
        glm::mat4 V = camera.GetV();
        glUniformMatrix4fv(phongShader["projectionMatrix"], 1,  GL_FALSE, glm::value_ptr(P));

        glUniform3fv(phongShader["Ia"], 1, glm::value_ptr(light.Ia));
        glUniform3fv(phongShader["Id"], 1, glm::value_ptr(light.Id));
        glUniform3fv(phongShader["Is"], 1, glm::value_ptr(light.Is));
        glm::vec3 lEye = glm::vec3(V * glm::vec4(light.direction, 0));
        glUniform3fv(phongShader["lightInEyeSpace"], 1, glm::value_ptr(lEye));

        gameObj.GetComponent<ModelRenderer>()->Render(camera);

        window.EndFrame();
        Input::PollEvents();
    }

    return 0;
}
