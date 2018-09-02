#include "progression.h"
// #include "primary_canvas.h"

using namespace Progression;

std::string rootDirectory;
// PrimaryCanvas* primaryCanvas;

// argv[1] = path of the root directory
int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Please pass in the path of the root directory as the first argument" << std::endl;
        return 0;
    }
    rootDirectory = argv[1];

    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::Window::Init(conf);
    PG::Time::Init(conf);
    Input::Init(conf);

    auto screen = PG::Window::getUIScreen();
    // PrimaryCanvas* primaryCanvas = new PrimaryCanvas(screen);

    screen->setVisible(true);
    screen->performLayout();

    // Game loop
    while (!Window::shouldClose()) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        const auto& bgColor = PG::Window::getBackgroundColor();
        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        // Draw UI & primary canvas
        screen->drawContents();
        screen->drawWidgets();

        PG::Window::EndFrame();
    }

    Input::Free();
    Time::Free();
    PG::Window::Free();

    return 0;
}
