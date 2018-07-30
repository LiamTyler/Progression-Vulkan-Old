#include "progression.h"
#include "nanogui/nanogui.h"
#include <Eigen/Dense>

using namespace std;
using namespace PG; // PG is a shortcut for Progression defined in progression.h

enum test_enum {
    Item1 = 0,
    Item2,
    Item3
};

bool bvar = true;
int ivar = 12345678;
double dvar = 3.1415926;
float fvar = (float)dvar;
std::string strval = "A string";
test_enum enumval = Item2;
nanogui::Color colval(0.5f, 0.5f, 0.7f, 1.f);

nanogui::Screen *screen = nullptr;

int SW = 800;
int SH = 600;

int main(int arc, char** argv) {
    Window window("OpenGL_Starter Example 2", SW, SH);

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

    // Create a nanogui screen and pass the glfw pointer to initialize
    screen = new nanogui::Screen;
    screen->initialize(window.getHandle(), true);

    // Create nanogui gui
    bool enabled = true;
    nanogui::FormHelper *gui = new nanogui::FormHelper(screen);
    nanogui::Window* nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Form helper example");
    gui->addGroup("Basic types");
    gui->addVariable("bool", bvar)->setTooltip("Test tooltip.");
    gui->addVariable("string", strval);

    gui->addGroup("Validating fields");
    gui->addVariable("int", ivar)->setSpinnable(true);
    gui->addVariable("float", fvar)->setTooltip("Test.");
    gui->addVariable("double", dvar)->setSpinnable(true);

    gui->addGroup("Complex types");
    gui->addVariable("Enumeration", enumval, enabled)->setItems({ "Item 1", "Item 2", "Item 3" });
    gui->addVariable("Color", colval)
        ->setFinalCallback([](const nanogui::Color &c) {
        std::cout << "ColorPicker Final Callback: ["
            << c.r() << ", "
            << c.g() << ", "
            << c.b() << ", "
            << c.w() << "]" << std::endl;
    });

    gui->addGroup("Other widgets");
    gui->addButton("A button", []() { std::cout << "Button pressed." << std::endl; })->setTooltip("Testing a much longer tooltip, that will wrap around to new lines multiple times.");
    nanoguiWindow->setPosition(Eigen::Vector2i(0, 0));

    screen->setVisible(true);
    screen->performLayout();
    // nanoguiWindow->center();

    glfwSetCursorPosCallback(window.getHandle(),
        [](GLFWwindow *, double x, double y) {
        screen->cursorPosCallbackEvent(x, y);
    }
    );

    glfwSetMouseButtonCallback(window.getHandle(),
        [](GLFWwindow *, int button, int action, int modifiers) {
        screen->mouseButtonCallbackEvent(button, action, modifiers);
    }
    );

    glfwSetKeyCallback(window.getHandle(),
        [](GLFWwindow *, int key, int scancode, int action, int mods) {
        screen->keyCallbackEvent(key, scancode, action, mods);
    }
    );

    glfwSetCharCallback(window.getHandle(),
        [](GLFWwindow *, unsigned int codepoint) {
        screen->charCallbackEvent(codepoint);
    }
    );

    glfwSetDropCallback(window.getHandle(),
        [](GLFWwindow *, int count, const char **filenames) {
        screen->dropCallbackEvent(count, filenames);
    }
    );

    glfwSetScrollCallback(window.getHandle(),
        [](GLFWwindow *, double x, double y) {
        screen->scrollCallbackEvent(x, y);
    }
    );

    glfwSetFramebufferSizeCallback(window.getHandle(),
        [](GLFWwindow *, int width, int height) {
        screen->resizeCallbackEvent(width, height);
    }
    );

    Model model("../../models/piano2.obj");
    model.Load();

    Background background(glm::vec4(.2, .2, .2, 1), nullptr);

    GameObject gameObj;
    gameObj.AddComponent<ModelRenderer>(new ModelRenderer(&phongShader, &model));

    // Game loop
    while (!glfwWindowShouldClose(window.getHandle())) {
        // Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
        glfwPollEvents();

        glEnable(GL_SCISSOR_TEST);
        glViewport(0, 0, 200, SH);
        glScissor(0, 0, 200, SH);

        // Draw nanogui
        glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        screen->drawContents();
        screen->drawWidgets();
        
        glViewport(200, 0, SW - 200, SH);
        glScissor(200, 0, SW - 200, SH);

        float dt = window.GetDT();
        camera.Update(dt);
        gameObj.Update(dt);

        background.ClearAndRender(camera);

        phongShader.Enable();
        glm::mat4 P = camera.GetP();
        glm::mat4 V = camera.GetV();
        glUniformMatrix4fv(phongShader["projectionMatrix"], 1, GL_FALSE, glm::value_ptr(P));

        glUniform3fv(phongShader["Ia"], 1, glm::value_ptr(light.Ia));
        glUniform3fv(phongShader["Id"], 1, glm::value_ptr(light.Id));
        glUniform3fv(phongShader["Is"], 1, glm::value_ptr(light.Is));
        glm::vec3 lEye = glm::vec3(V * glm::vec4(light.direction, 0));
        glUniform3fv(phongShader["lightInEyeSpace"], 1, glm::value_ptr(lEye));

        gameObj.GetComponent<ModelRenderer>()->Render(camera);
        glDisable(GL_SCISSOR_TEST);



        glfwSwapBuffers(window.getHandle());
    }

    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();

    return 0;
}
