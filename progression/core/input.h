#pragma once

#include "core/common.h"
#include "core/config.h"
#include "core/input_types.h"

namespace Progression {

    class Input {
    public:
        Input() = delete;
        ~Input() = delete;

        static void Init(const config::Config& config);
        static void Free() { glfwPollEvents(); }
        static void PollEvents();

        static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
        static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
        static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
        static bool GetKeyDown(Key k);
        static bool GetKeyUp(Key k);
        static bool GetMouseButtonDown(MouseButton b);
        static bool GetMouseButtonUp(MouseButton b);
        static glm::ivec2 GetMousePosition();
        static glm::ivec2 GetMouseChange();
        static glm::ivec2 GetScrollChange();

    private:

        static bool keysDown_[GLFW_KEY_LAST + 1];
        static bool keysUp_[GLFW_KEY_LAST + 1];
        static bool mouseButtonDown_[GLFW_MOUSE_BUTTON_LAST + 1];
        static bool mouseButtonUp_[GLFW_MOUSE_BUTTON_LAST + 1];
        static glm::ivec2 lastCursorPos_;
        static glm::ivec2 currentCursorPos_;
        static glm::ivec2 scrollOffset_;
    };

} // namespace Progression