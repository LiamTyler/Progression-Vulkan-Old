#include "include/window.h"
#include "include/input.h"

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

namespace Progression {

	Window::Window(const std::string& title, int w, int h, bool vsync) :
		title_(title),
        screenWidth_(w),
        screenHeight_(h),
		fpsCounter_(),
        window_(nullptr),
        vsync_(vsync)
	{
		Init();
	}

	Window::~Window() {
        if (window_)
            glfwDestroyWindow(window_);
        glfwTerminate();
	}

	void Window::Init() {
        if (!glfwInit()) {
            std::cout << "Could not initialize GLFW" << std::endl;
            exit(EXIT_FAILURE);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        window_ = glfwCreateWindow(screenWidth_, screenHeight_, title_.c_str(), NULL, NULL);
        if (!window_) {
            std::cout << "Window or context creation failed" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwMakeContextCurrent(window_);

        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
            std::cout << "Failed to gladLoadGLLoader" << std::endl;
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
		
		std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
		std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
		std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;

        if (vsync_) {
            glfwSwapInterval(1);
        }

        glfwGetFramebufferSize(window_, &viewportWidth_, &viewportHeight_);
        glViewport(0, 0, viewportWidth_, viewportHeight_);

        // setup the callbacks
        Input::Init(window_);
        glfwSetKeyCallback(window_, Input::key_callback);
        glfwSetCursorPosCallback(window_, Input::cursor_position_callback);
        glfwSetMouseButtonCallback(window_, Input::mouse_button_callback);
        glfwSetScrollCallback(window_, Input::scroll_callback);

		glEnable(GL_DEPTH_TEST);
	}

	void Window::SwapWindow() {
        glfwSwapBuffers(window_);
	}

	void Window::SetRelativeMouse(bool b) {
        if (b)
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        else
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	float Window::GetTotalRuntime() const {
        return glfwGetTime();
	}

	float Window::GetDT() const {
		return fpsCounter_.GetDT();
	}

	void Window::StartFrame() {
		fpsCounter_.StartFrame(GetTotalRuntime());
	}

	void Window::EndFrame() {
		fpsCounter_.EndFrame();
		SwapWindow();
	}

} // namespace Progression