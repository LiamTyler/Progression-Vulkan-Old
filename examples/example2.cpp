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

int main(int arc, char** argv) {
    Window window("OpenGL_Starter Example 2", 800, 600);

    // Create a nanogui screen and pass the glfw pointer to initialize
    screen = new nanogui::Screen;
    screen->initialize(window.getHandle(), true);

    int width, height;
    glfwGetFramebufferSize(window.getHandle(), &width, &height);
    glViewport(0, 0, width, height);
    glfwSwapInterval(0);
    glfwSwapBuffers(window.getHandle());

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
    gui->addButton("A button", []() { std::cout << "Button pressed." << std::endl; })->setTooltip("Testing a much longer tooltip, that will wrap around to new lines multiple times.");;

    screen->setVisible(true);
    screen->performLayout();
    nanoguiWindow->center();

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

    // Game loop
    while (!glfwWindowShouldClose(window.getHandle())) {
        // Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
        glfwPollEvents();

        glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw nanogui
        screen->drawContents();
        screen->drawWidgets();

        glfwSwapBuffers(window.getHandle());
    }

    // Terminate GLFW, clearing any resources allocated by GLFW.
    glfwTerminate();

    return 0;
}
