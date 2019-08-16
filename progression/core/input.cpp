#include "core/input.hpp"
#include "core/window.hpp"

static bool s_keysDown[GLFW_KEY_LAST + 1]                 = { 0 };
static bool s_keysUp[GLFW_KEY_LAST + 1]                   = { 0 };
static bool s_mouseButtonDown[GLFW_MOUSE_BUTTON_LAST + 1] = { 0 };
static bool s_mouseButtonUp[GLFW_MOUSE_BUTTON_LAST + 1]   = { 0 };
static glm::ivec2 s_lastCursorPos                         = glm::ivec2( 0 );
static glm::ivec2 s_currentCursorPos                      = glm::ivec2( 0 );
static glm::ivec2 s_scrollOffset                          = glm::ivec2( 0 );

static void KeyCallback( GLFWwindow* window, int key, int scancode, int action, int mods )
{
    (void) window;
    (void) scancode;
    (void) mods;
    if ( action == GLFW_PRESS )
    {
        s_keysDown[key] = true;
    }
    else if ( action == GLFW_RELEASE )
    {
        s_keysUp[key]   = true;
        s_keysDown[key] = false;
    }
}

static void CursorPositionCallback( GLFWwindow* window, double xpos, double ypos )
{
    (void) window;
    s_currentCursorPos = glm::ivec2( xpos, ypos );
}

static void MouseButtonCallback( GLFWwindow* window, int button, int action, int mods )
{
    (void) window;
    (void) mods;
    if ( action == GLFW_PRESS )
    {
        s_mouseButtonDown[button] = true;
    }
    else if ( action == GLFW_RELEASE )
    {
        s_mouseButtonUp[button]   = true;
        s_mouseButtonDown[button] = false;
    }
}

static void ScrollCallback( GLFWwindow* window, double xoffset, double yoffset )
{
    (void) window;
    s_scrollOffset += glm::ivec2( xoffset, yoffset );
}


namespace Progression
{
namespace Input
{

    void Init()
    {
        GLFWwindow* window = GetMainWindow()->GetGLFWHandle();
        double x, y;
        glfwGetCursorPos( window, &x, &y );
        s_currentCursorPos = glm::ivec2( x, y );
        s_lastCursorPos    = s_currentCursorPos;

        glfwSetCursorPosCallback( window, CursorPositionCallback );
        glfwSetMouseButtonCallback( window, MouseButtonCallback );
        glfwSetKeyCallback( window, KeyCallback );
        glfwSetScrollCallback( window, ScrollCallback );

        PollEvents();
    }

    void Free()
    {
        glfwPollEvents();
    }

    void PollEvents()
    {
        // Reset all of the states for the next frame
        memset( s_keysDown, false, sizeof( s_keysDown ) );
        memset( s_keysUp, false, sizeof( s_keysUp ) );
        memset( s_mouseButtonDown, false, sizeof( s_mouseButtonDown ) );
        memset( s_mouseButtonUp, false, sizeof( s_mouseButtonUp ) );
        s_lastCursorPos = s_currentCursorPos;
        s_scrollOffset  = glm::ivec2( 0 );
        glfwPollEvents();
    }

    bool GetKeyDown( Key k )
    {
        return s_keysDown[k];
    }

    bool GetKeyUp( Key k )
    {
        return s_keysUp[k];
    }

    bool GetMouseButtonDown( MouseButton b )
    {
        return s_mouseButtonDown[b];
    }

    bool GetMouseButtonUp( MouseButton b )
    {
        return s_mouseButtonUp[b];
    }

    glm::ivec2 GetMousePosition()
    {
        return s_currentCursorPos;
    }

    glm::ivec2 GetMouseChange()
    {
        return s_currentCursorPos - s_lastCursorPos;
    }

    glm::ivec2 GetScrollChange()
    {
        return s_scrollOffset;
    }

} // namespace Input
} // namespace Progression
