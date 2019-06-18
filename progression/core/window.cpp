#include "core/window.hpp"
#include "core/time.hpp"
#include "utils/logger.hpp"

// debug context log function just copied from learnopengl.com
static void APIENTRY glDebugOutput(GLenum source,
                            GLenum type,
                            GLuint id,
                            GLenum severity,
                            GLsizei length,
                            const GLchar *message,
                            const void *userParam)
{
    UNUSED(length);
    UNUSED(userParam);
    // ignore non-significant error/warning codes
    if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

    std::cout << "---------------" << std::endl;
    std::cout << "Debug message (" << id << "): " <<  message << std::endl;

    switch (source)
    {
        case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
        case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
    } std::cout << std::endl;

    switch (type)
    {
        case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
        case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
        case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
        case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
    } std::cout << std::endl;

    switch (severity)
    {
        case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
        case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
        case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
        case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
    } std::cout << std::endl;
    std::cout << std::endl;
}

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
            LOG_ERR("Could not initialize GLFW");
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
		bool vsync;
        bool debugContext;

        // Grab the specified values in the config if available
		auto winConfig = config->get_table("window");
        if (!winConfig) {
            LOG_ERR("Need to specify the 'window' in the config file");
            exit(EXIT_FAILURE);
        }

		glMajor = winConfig->get_as<int>("glMajor").value_or(4);
        glMinor = winConfig->get_as<int>("glMinor").value_or(3);
        if (glMajor * 10 + glMinor < 43)
            LOG_WARN("Using a version of OpenGL less that 4.3. Tiled deferred rendering relies on compute shaders, which is version 4.3 and up");

		// resizeable = winConfig->get_as<bool>("resizeable").value_or(false);
		_mTitle        = winConfig->get_as<std::string>("title").value_or("untitled");
		_mWindowSize.x = winConfig->get_as<int>("width").value_or(640);
		_mWindowSize.y = winConfig->get_as<int>("height").value_or(480);
		vsync          = winConfig->get_as<bool>("vsync").value_or(false);
		debugContext   = winConfig->get_as<bool>("debugContext").value_or(false);
		

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, glMajor);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, glMinor);        
        glfwWindowHint(GLFW_RESIZABLE, false);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, debugContext);


        _mWindow = glfwCreateWindow(_mWindowSize.x, _mWindowSize.y, _mTitle.c_str(), NULL, NULL);
        if (!_mWindow) {
            LOG_ERR("Window or context creation failed");
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        glfwMakeContextCurrent(_mWindow);

        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
            LOG_ERR("Failed to gladLoadGLLoader");
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
		
        LOG("Vendor:", glGetString(GL_VENDOR));
        LOG("Renderer:", glGetString(GL_RENDERER));
        LOG("Version:", glGetString(GL_VERSION));

        if (vsync) {
            glfwSwapInterval(1);
        } else {
            glfwSwapInterval(0);
        }
        glfwSwapInterval(1);

        int fbW, fbH, wW, wH;
        glfwGetFramebufferSize(_mWindow, &fbW, &fbH);
        glfwGetWindowSize(_mWindow, &wW, &wH);

        glViewport(0, 0, fbW, fbH);

        if (debugContext) {
            GLint flags; glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
            if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
                LOG("enabling debug context");
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                glDebugMessageCallback(glDebugOutput, nullptr);
                glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
            }
        }
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

	void Window::setTitle(const std::string& title) {
		glfwSetWindowTitle(_mWindow, title.c_str());
	}


} // namespace Progression
