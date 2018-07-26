#pragma once

#include "include/input_types.h"

namespace Progression {

    class Input {
    public:
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;

        static void PollEvents();

        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

        static bool GetKeyDown(Key k);
        static bool GetKeyUp(Key k);
        static glm::vec2 GetMousePosition();
        static glm::vec2 GetMouseChange();


    private:
        Input() {}

        static bool keysDown_[GLFW_KEY_LAST + 1];
        static bool keysUp_[GLFW_KEY_LAST + 1];
        static glm::vec2 lastCursorPos_;
        static glm::vec2 currentCursorPos_;
    };

} // namespace Progression