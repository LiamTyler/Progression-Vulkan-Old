#include "include/input.h"

namespace Progression {

    bool Input::keysDown_[] = { 0 };
    bool Input::keysUp_[] = { 0 };
    glm::vec2 lastCursorPos_ = glm::vec2(0);
    glm::vec2 currentCursorPos_ = glm::vec2(0);

    void Input::PollEvents() {
        memset(keysDown_, false, sizeof(keysDown_));
        memset(keysUp_, false, sizeof(keysUp_));
        glfwPollEvents();
    }

    void Input::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        // std::cout << "in key_callback: " << key << " " << action << std::endl;
        if (action == GLFW_PRESS) {
            keysDown_[key] = true;
        } else if (action == GLFW_RELEASE) {
            keysUp_[key] = true;
            keysDown_[key] = false;
        }
    }

    void Input::cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
        std::cout << "in cursor_callback: " << xpos << " " << ypos << std::endl;
        lastCursorPos_ = currentCursorPos_;
        currentCursorPos_ = glm::vec2(xpos, ypos);
    }

    bool Input::GetKeyDown(Key k) {
        return keysDown_[k];
    }

    bool Input::GetKeyUp(Key k) {
        return keysUp_[k];
    }

    glm::vec2 Input::GetMousePosition() {
        return currentCursorPos_;
    }

    glm::vec2 Input::GetMouseChange() {
        return currentCursorPos_ - lastCursorPos_;
    }

} // namespace Progression