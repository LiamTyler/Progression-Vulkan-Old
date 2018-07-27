#include "include/input.h"

namespace Progression {

    bool Input::keysDown_[] = { 0 };
    bool Input::keysUp_[] = { 0 };
    glm::ivec2 Input::lastCursorPos_ = glm::ivec2(0);
    glm::ivec2 Input::currentCursorPos_ = glm::ivec2(0);

    void Input::Init(GLFWwindow* window) {
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        currentCursorPos_ = glm::ivec2(x, y);
        lastCursorPos_ = currentCursorPos_;
    }

    void Input::PollEvents() {
        memset(keysDown_, false, sizeof(keysDown_));
        memset(keysUp_, false, sizeof(keysUp_));
        lastCursorPos_ = currentCursorPos_;
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
        currentCursorPos_ = glm::ivec2(xpos, ypos);
    }

    bool Input::GetKeyDown(Key k) {
        return keysDown_[k];
    }

    bool Input::GetKeyUp(Key k) {
        return keysUp_[k];
    }

    glm::ivec2 Input::GetMousePosition() {
        return currentCursorPos_;
    }

    glm::ivec2 Input::GetMouseChange() {
        return currentCursorPos_ - lastCursorPos_;
    }

} // namespace Progression