#include "core/window.h"
#include "core/time.h"

namespace Progression {

    GLFWwindow* Window::_mWindow = nullptr;

    std::string Window::_mTitle = "";
    glm::ivec2 Window::_mWindowSize = { 0, 0 };

	void Window::Free() {
        if (_mWindow)
            glfwDestroyWindow(_mWindow);

        glfwTerminate();
        _mWindow = nullptr;
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
		int glMajor;
		int glMinor;
		bool resizeable;
		bool vsync;

        // Grab the specified values in the config if available
		auto winConfig = config->get_table("window");
		if (!winConfig)
			std::cout << "Need to specify the window subsystem in the config file!" << std::endl;

		glMajor = winConfig->get_as<int>("glMajor").value_or(4);
        glMinor = winConfig->get_as<int>("glMinor").value_or(3);
		resizeable = winConfig->get_as<bool>("resizeable").value_or(false);
		_mTitle = winConfig->get_as<std::string>("title").value_or("untitled");
		_mWindowSize.x = winConfig->get_as<int>("width").value_or(640);
		_mWindowSize.y = winConfig->get_as<int>("height").value_or(480);
		vsync = winConfig->get_as<bool>("vsync").value_or(false);
		

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
        } else {
            glfwSwapInterval(0);
        }

        int fbW, fbH, wW, wH;
        glfwGetFramebufferSize(_mWindow, &fbW, &fbH);
        glfwGetWindowSize(_mWindow, &wW, &wH);

        glViewport(0, 0, fbW, fbH);
        // glBindSampler(0, 0);
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

    void Window::SetRelativeMouse(bool b) {
        if (b) {
            glfwSetInputMode(_mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(_mWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

} // namespace Progression
