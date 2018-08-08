#include "include/window.h"
#include "core/time.h"

namespace Progression {

    GLFWwindow* Window::_mWindow = nullptr;
    nanogui::Screen* Window::_mUIScreen = nullptr;

    std::string Window::_mTitle = "";
    glm::ivec2 Window::_mWindowSize = { 0, 0 };

	void Window::Free() {
        if (_mUIScreen)
            delete _mUIScreen;

        if (_mWindow)
            glfwDestroyWindow(_mWindow);

        glfwTerminate();
        _mWindow = nullptr;
        _mUIScreen = nullptr;
	}

	void Window::Init(const config::Config& config) {
        if (!glfwInit()) {
            std::cout << "Could not initialize GLFW" << std::endl;
            exit(EXIT_FAILURE);
        }

        // default non-configurable settings
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 0);
        glfwWindowHint(GLFW_RED_BITS, 8);
        glfwWindowHint(GLFW_GREEN_BITS, 8);
        glfwWindowHint(GLFW_BLUE_BITS, 8);
        glfwWindowHint(GLFW_ALPHA_BITS, 8);
        glfwWindowHint(GLFW_STENCIL_BITS, 8);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);

        // default configurable settings
        int glMajor = 4;
        int glMinor = 3;
        bool resizeable = true;
        bool vsync = true;
        _mTitle = "Untitled";
        _mWindowSize = { 640, 480 };

        // Grab the specified values in the config if available
        auto& winConfig = config["window"];
        if (winConfig) {
            if (winConfig["glMajor"])
                glMajor = winConfig["glMajor"].as<int>();
            if (winConfig["glMinor"])
                glMinor = winConfig["glMinor"].as<int>();
            if (winConfig["resizeable"])
                resizeable = winConfig["resizeable"].as<bool>();
            if (winConfig["title"])
                _mTitle = winConfig["title"].as<std::string>();
            if (winConfig["width"])
                _mWindowSize.x = winConfig["width"].as<int>();
            if (winConfig["height"])
                _mWindowSize.y = winConfig["height"].as<int>();
            if (winConfig["vsync"])
                vsync = winConfig["vsync"].as<bool>();
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glMajor);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glMinor);        
        glfwWindowHint(GLFW_RESIZABLE, resizeable);

        _mWindow = glfwCreateWindow(_mWindowSize.x, _mWindowSize.y, _mTitle.c_str(), NULL, NULL);
        if (!_mWindow) {
            std::cout << "Window or context creation failed" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwMakeContextCurrent(_mWindow);

        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
            std::cout << "Failed to gladLoadGLLoader" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
		
		std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
		std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
		std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;

        if (vsync) {
            glfwSwapInterval(1);
        }

        _mUIScreen = new nanogui::Screen;
        _mUIScreen->initialize(_mWindow, false);
	}

	void Window::SwapWindow() {
        glfwSwapBuffers(_mWindow);
	}

	void Window::StartFrame() {
        Time::StartFrame();
	}

	void Window::EndFrame() {
        Time::EndFrame();
		SwapWindow();
	}

} // namespace Progression