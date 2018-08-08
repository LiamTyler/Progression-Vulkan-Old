#include "progression.h"
#include <iostream>

using namespace nanogui;
using namespace Progression;

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
Color colval(0.5f, 0.5f, 0.7f, 1.f);

Screen *screen = nullptr;

int main(int /* argc */, char ** /* argv */) {

    auto& conf = PG::config::Config("C:/Users/Tyler/Documents/Progression/configs/test.yaml");

    PG::Window::Init(conf);
    PG::Time::Init();
    Input::Init();

    // Create nanogui gui
    auto screen = PG::Window::getUIScreen();
    bool enabled = true;
    FormHelper *gui = new FormHelper(screen);
    ref<nanogui::Window> nanoguiWindow = gui->addWindow(Eigen::Vector2i(10, 10), "Form helper example");
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
        ->setFinalCallback([](const Color &c) {
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

    // Game loop
    while (!glfwWindowShouldClose(PG::Window::getGLFWHandle())) {
        // Check if any events have been activated (key pressed, mouse moved etc.) and call corresponding response functions
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Draw nanogui
        screen->drawContents();
        screen->drawWidgets();

        PG::Window::EndFrame();
    }

    Input::Free();
    std::cout << "after input free" << std::endl;
    Time::Free();
    std::cout << "after time free" << std::endl;
    PG::Window::Free();
    std::cout << "after window free" << std::endl;

    return 0;
}