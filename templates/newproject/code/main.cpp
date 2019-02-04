#include "##projectName##.hpp"
#include "progression.hpp"
#include "configuration.hpp"

int main(int argc, char* argv[]) {
	PG::EngineInitialize(ROOT_DIR "configs/##projectName##.toml");
    PG::Window::SetRelativeMouse(true);
	PG::Input::PollEvents();

    if (!##projectName##::init()) {
        LOG_ERR("Failed to initialize the project, exiting.");
        exit(EXIT_FAILURE);
    }

	// Game loop
	while (!PG::EngineShutdown) {
		PG::Window::StartFrame();
		PG::Input::PollEvents();


        ##projectName##::handleInput();

        ##projectName##::update();

        ##projectName##::render();


		PG::Window::EndFrame();
	}

    ##projectName##::shutdown();
	PG::EngineQuit();

	return 0;
}
