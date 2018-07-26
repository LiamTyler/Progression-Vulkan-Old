#pragma once

#include "include/input_types.h"

namespace Progression {

    class Input {
    public:
        Input(const Input&) = delete;
        Input& operator=(const Input&) = delete;

        static void PollEvents();

        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

        static bool GetKeyDown(Key k);
        static bool GetKeyUp(Key k);


    private:
        Input() {}

        static bool keysDown_[GLFW_KEY_LAST + 1];
        static bool keysUp_[GLFW_KEY_LAST + 1];

    };

} // namespace Progression