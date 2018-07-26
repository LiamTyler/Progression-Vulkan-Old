#include "include/input.h"

namespace Progression {

    bool Input::keysDown_[] = { 0 };
    bool Input::keysUp_[] = { 0 };

    void Input::PollEvents() {
        memset(keysDown_, false, sizeof(keysDown_));
        memset(keysUp_, false, sizeof(keysUp_));
        glfwPollEvents();
    }

    void Input::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        std::cout << "in key_callback: " << key << " " << action << std::endl;
        if (action == GLFW_PRESS) {
            keysDown_[key] = true;
        } else if (action == GLFW_RELEASE) {
            keysUp_[key] = true;
            keysDown_[key] = false;
        }
    }

    bool Input::GetKeyDown(Key k) {
        return keysDown_[k];
    }

    bool Input::GetKeyUp(Key k) {
        return keysUp_[k];
    }

} // namespace Progression