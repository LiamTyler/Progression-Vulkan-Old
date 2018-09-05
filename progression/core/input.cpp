#include "core/input.h"
#include "core/window.h"

namespace Progression {

    bool Input::keysDown_[] = { 0 };
    bool Input::keysUp_[] = { 0 };
    bool Input::mouseButtonDown_[] = { 0 };
    bool Input::mouseButtonUp_[] = { 0 };
    glm::ivec2 Input::lastCursorPos_ = glm::ivec2(0);
    glm::ivec2 Input::currentCursorPos_ = glm::ivec2(0);
    glm::ivec2 Input::scrollOffset_ = glm::ivec2(0);

    void Input::Init(const config::Config& config) {
        GLFWwindow* window = Window::getGLFWHandle();
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        currentCursorPos_ = glm::ivec2(x, y);
        lastCursorPos_ = currentCursorPos_;

        glfwSetCursorPosCallback(window,
            [](GLFWwindow *window, double x, double y) {
            Window::getUIScreen()->cursorPosCallbackEvent(x, y);
            cursor_position_callback(window, x, y);
        }
        );

        glfwSetMouseButtonCallback(window,
            [](GLFWwindow *window, int button, int action, int modifiers) {
            Window::getUIScreen()->mouseButtonCallbackEvent(button, action, modifiers);
            mouse_button_callback(window, button, action, modifiers);
        }
        );

        glfwSetKeyCallback(window,
            [](GLFWwindow *window, int key, int scancode, int action, int mods) {
            Window::getUIScreen()->keyCallbackEvent(key, scancode, action, mods);
            key_callback(window, key, scancode, action, mods);
        }
        );

        glfwSetCharCallback(window,
            [](GLFWwindow *window, unsigned int codepoint) {
            Window::getUIScreen()->charCallbackEvent(codepoint);
        }
        );

        glfwSetDropCallback(window,
            [](GLFWwindow *window, int count, const char **filenames) {
            Window::getUIScreen()->dropCallbackEvent(count, filenames);
        }
        );

        glfwSetScrollCallback(window,
            [](GLFWwindow *window, double x, double y) {
            Window::getUIScreen()->scrollCallbackEvent(x, y);
            scroll_callback(window, x, y);
        }
        );

        glfwSetFramebufferSizeCallback(window,
            [](GLFWwindow *window, int width, int height) {
            Window::getUIScreen()->resizeCallbackEvent(width, height);
        }
        );

        PollEvents();
    }

    void Input::PollEvents() {
        // Reset all of the states for the next frame
        memset(keysDown_, false, sizeof(keysDown_));
        memset(keysUp_, false, sizeof(keysUp_));
        memset(mouseButtonDown_, false, sizeof(mouseButtonDown_));
        memset(mouseButtonUp_, false, sizeof(mouseButtonUp_));
        lastCursorPos_ = currentCursorPos_;
        scrollOffset_ = glm::ivec2(0);
        glfwPollEvents();
    }

    void Input::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        if (action == GLFW_PRESS) {
            keysDown_[key] = true;
        } else if (action == GLFW_RELEASE) {
            keysUp_[key] = true;
            keysDown_[key] = false;
        }
    }

    void Input::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
        currentCursorPos_ = glm::ivec2(xpos, ypos);
    }

    void Input::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
        if (action == GLFW_PRESS) {
            mouseButtonDown_[button] = true;
        }
        else if (action == GLFW_RELEASE) {
            mouseButtonUp_[button] = true;
            mouseButtonDown_[button] = false;
        }
    }

    void Input::scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
        scrollOffset_ += glm::ivec2(xoffset, yoffset);
    }

    bool Input::GetKeyDown(Key k) {
        return keysDown_[k];
    }

    bool Input::GetKeyUp(Key k) {
        return keysUp_[k];
    }

    bool Input::GetMouseButtonDown(MouseButton b) {
        return mouseButtonDown_[b];
    }

    bool Input::GetMouseButtonUp(MouseButton b) {
        return mouseButtonUp_[b];
    }

    glm::ivec2 Input::GetMousePosition() {
        return currentCursorPos_;
    }

    glm::ivec2 Input::GetMouseChange() {
        return currentCursorPos_ - lastCursorPos_;
    }

    glm::ivec2 Input::GetScrollChange() {
        return scrollOffset_;
    }

} // namespace Progression