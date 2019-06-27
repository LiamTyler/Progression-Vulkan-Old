#include "core/window.hpp"
#include "core/time.hpp"
#include "utils/logger.hpp"
#include <mutex>
#include <array>

#define BACKGROUND_WINDOWS 10

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

static void errorCallback(int err, const char* description) {
    LOG("GLFW ERROR: ", err, ", ", description);
}

namespace Progression {

    namespace {

        Window* mainWindow = nullptr;
        std::mutex glfwInitLock;
        std::array<Window, BACKGROUND_WINDOWS> backgroundWindows;

    } // namespace anonymous

    // TODO: take in config info
    void initWindowSystem() {
        if (!glfwInit()) {
            LOG_ERR("Could not initialize GLFW");
            exit(EXIT_FAILURE);
        }

        mainWindow = new Window;
        WindowCreateInfo info;
        mainWindow->init(info);
        // mainWindow->unbindContext();

        // info.width = 1;
        // info.height = 1;
        // info.visible = false;
        // for (auto& window : backgroundWindows) {
        //     window.init(info);
        //     LOG("created bg window");
        // }
        mainWindow->bindContext();
    }

    void shutdownWindowSystem() {
        delete mainWindow;
        glfwTerminate();
    }

    Window* getMainWindow() {
        return mainWindow;
    }

    Window::~Window() {
        if (window_) {
            glfwDestroyWindow(window_);
            window_ = nullptr;
        }
    }

	void Window::init(const struct WindowCreateInfo& createInfo) {
        title_ = createInfo.title;
        width_ = createInfo.width;
        height_ = createInfo.height;
        visible_ = createInfo.visible;

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

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);        
        glfwWindowHint(GLFW_RESIZABLE, false);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, createInfo.debugContext);

        glfwWindowHint(GLFW_VISIBLE, visible_);
        GLFWwindow* parentWindow = getMainWindow() == this ? NULL : getMainWindow()->getGLFWHandle();
        window_ = glfwCreateWindow(width_, height_, title_.c_str(), NULL, parentWindow);
        if (!window_) {
            LOG_ERR("Window or context creation failed");
            glfwTerminate();
            exit(EXIT_FAILURE);
        }

        if (parentWindow)
            return;

        bindContext();

        glfwSetErrorCallback(errorCallback);

        if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
            LOG_ERR("Failed to gladLoadGLLoader");
            glfwTerminate();
            exit(EXIT_FAILURE);
        }
		
        if (getMainWindow() == this) {
            LOG("Vendor:", glGetString(GL_VENDOR));
            LOG("Renderer:", glGetString(GL_RENDERER));
            LOG("Version:", glGetString(GL_VERSION));
        }

        if (createInfo.vsync) {
            glfwSwapInterval(1);
        } else {
            glfwSwapInterval(0);
        }

        int fbW, fbH, wW, wH;
        glfwGetFramebufferSize(window_, &fbW, &fbH);
        glfwGetWindowSize(window_, &wW, &wH);

        glViewport(0, 0, fbW, fbH);

        if (createInfo.debugContext) {
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

    void Window::bindContext() {
        glfwMakeContextCurrent(window_);
    }

    void Window::unbindContext() {
        glfwMakeContextCurrent(NULL);
    }

	void Window::swapWindow() {
        glfwSwapBuffers(window_);
	}

	void Window::startFrame() {
        Time::StartFrame();
	}

	void Window::endFrame() {
        Time::EndFrame();
		swapWindow();
	}

    void Window::setRelativeMouse(bool b) {
        if (b) {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(window_, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

	void Window::setTitle(const std::string& title) {
        title_ = title;
		glfwSetWindowTitle(window_, title_.c_str());
	}


} // namespace Progression
