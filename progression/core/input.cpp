#include "core/input.hpp"
#include "core/window.hpp"
#include "core/lua.hpp"

static bool s_keysDown[GLFW_KEY_LAST + 1]                 = { 0 };
static bool s_keysUp[GLFW_KEY_LAST + 1]                   = { 0 };
static bool s_mouseButtonDown[GLFW_MOUSE_BUTTON_LAST + 1] = { 0 };
static bool s_mouseButtonUp[GLFW_MOUSE_BUTTON_LAST + 1]   = { 0 };
static glm::vec2 s_lastCursorPos                         = glm::ivec2( 0 );
static glm::vec2 s_currentCursorPos                      = glm::ivec2( 0 );
static glm::vec2 s_scrollOffset                          = glm::ivec2( 0 );

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
    s_currentCursorPos = glm::vec2( xpos, ypos );
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
    s_scrollOffset += glm::vec2( xoffset, yoffset );
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
        s_currentCursorPos = glm::vec2( x, y );
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
        s_scrollOffset  = glm::vec2( 0 );
        glfwPollEvents();
    }

    bool GetKeyDown( Key k )
    {
        return s_keysDown[static_cast< int >( k )];
    }

    bool GetKeyUp( Key k )
    {
        return s_keysUp[static_cast< int >( k )];
    }

    bool GetMouseButtonDown( MouseButton b )
    {
        return s_mouseButtonDown[static_cast< int >( b )];
    }

    bool GetMouseButtonUp( MouseButton b )
    {
        return s_mouseButtonUp[static_cast< int >( b )];
    }

    glm::vec2 GetMousePosition()
    {
        return s_currentCursorPos;
    }

    glm::vec2 GetMouseChange()
    {
        return s_currentCursorPos - s_lastCursorPos;
    }

    glm::vec2 GetScrollChange()
    {
        return s_scrollOffset;
    }

} // namespace Input
} // namespace Progression


void RegisterLuaFunctions_Input( lua_State* L )
{
    using namespace Progression;
    using namespace Progression::Input;

    sol::state_view lua(L);
    auto input = lua["Input"].get_or_create< sol::table >();
    input.set_function( "GetKeyDown", &GetKeyDown );
    input.set_function( "GetKeyUp", &GetKeyUp );
    input.set_function( "GetMouseButtonDown", &GetMouseButtonDown );
    input.set_function( "GetMouseButtonUp", &GetMouseButtonUp );
    input.set_function( "GetMousePosition", &GetMousePosition );
    input.set_function( "GetMouseChange", &GetMouseChange );
    input.set_function( "GetScrollChange", &GetScrollChange );

    std::initializer_list< std::pair< sol::string_view, Key > > keyItems =
    {
        { "A", Key::A },
        { "B", Key::B },
        { "C", Key::C },
        { "D", Key::D },
        { "E", Key::E },
        { "F", Key::F },
        { "H", Key::G },
        { "G", Key::H },
        { "I", Key::I },
        { "J", Key::J },
        { "K", Key::K },
        { "L", Key::L },
        { "M", Key::M },
        { "N", Key::N },
        { "O", Key::O },
        { "P", Key::P },
        { "Q", Key::Q },
        { "R", Key::R },
        { "S", Key::S },
        { "T", Key::T },
        { "U", Key::U },
        { "V", Key::V },
        { "W", Key::W },
        { "X", Key::X },
        { "Y", Key::Y },
        { "Z", Key::Z },
        { "UNKOWN", Key::UNKOWN },
        { "SPACE", Key::SPACE },
        { "ESC", Key::ESC },
        { "APOSTROPHE", Key::APOSTROPHE },
        { "COMMA", Key::COMMA },
        { "MINUS", Key::MINUS },
        { "PERIOD", Key::PERIOD },
        { "SLASH", Key::SLASH },
        { "SEMICOLON", Key::SEMICOLON },
        { "EQUAL", Key::EQUAL },
        { "LEFT_BRACKET", Key::LEFT_BRACKET },
        { "BACKSLASH", Key::BACKSLASH },
        { "RIGHT_BRACKET", Key::RIGHT_BRACKET },
        { "BACK_TICK", Key::BACK_TICK },
        { "ENTER", Key::ENTER },
        { "TAB", Key::TAB },
        { "BACKSPACE", Key::BACKSPACE },
        { "INSERT", Key::INSERT },
        { "DELETE", Key::DELETE },
        { "RIGHT", Key::RIGHT },
        { "LEFT", Key::LEFT },
        { "DOWN", Key::DOWN },
        { "UP", Key::UP },
        { "PAGE_UP", Key::PAGE_UP },
        { "PAGE_DOWN", Key::PAGE_DOWN },
        { "HOME", Key::HOME },
        { "END", Key::END },
        { "CAPS_LOCK", Key::CAPS_LOCK },
        { "SCROLL_LOCK", Key::SCROLL_LOCK },
        { "NUM_LOCK", Key::NUM_LOCK },
        { "PRINT_SCREEN", Key::PRINT_SCREEN },
        { "PAUSE", Key::PAUSE },
        { "LEFT_SHIFT", Key::LEFT_SHIFT },
        { "LEFT_CONTROL", Key::LEFT_CONTROL },
        { "LEFT_ALT", Key::LEFT_ALT },
        { "LEFT_SUPER", Key::LEFT_SUPER },
        { "RIGHT_SHIFT", Key::RIGHT_SHIFT },
        { "RIGHT_CONTROL", Key::RIGHT_CONTROL },
        { "RIGHT_ALT", Key::RIGHT_ALT },
        { "RIGHT_SUPER", Key::RIGHT_SUPER },
        { "MENU", Key::MENU },
        { "F1", Key::F1 },
        { "F2", Key::F2 },
        { "F3", Key::F3 },
        { "F4", Key::F4 },
        { "F5", Key::F5 },
        { "F6", Key::F6 },
        { "F7", Key::F7 },
        { "F8", Key::F8 },
        { "F9", Key::F9 },
        { "F10", Key::F10 },
        { "F11", Key::F11 },
        { "F12", Key::F12 },
        { "KP_0", Key::KP_0 },
        { "KP_1", Key::KP_1 },
        { "KP_2", Key::KP_2 },
        { "KP_3", Key::KP_3 },
        { "KP_4", Key::KP_4 },
        { "KP_5", Key::KP_5 },
        { "KP_6", Key::KP_6 },
        { "KP_7", Key::KP_7 },
        { "KP_8", Key::KP_8 },
        { "KP_9", Key::KP_9 },
        { "KP_DECIMAL",  Key::KP_DECIMAL },
        { "KP_DIVIDE",   Key::KP_DIVIDE },
        { "KP_MULTIPLY", Key::KP_MULTIPLY },
        { "KP_SUBTRACT", Key::KP_SUBTRACT },
        { "KP_ADD",      Key::KP_ADD },
        { "KP_ENTER",    Key::KP_ENTER },
        { "KP_EQUAL",    Key::KP_EQUAL }
    };
    lua.new_enum< Key, false >( "Key", keyItems ); // false makes it read/write in Lua, but its faster

    std::initializer_list< std::pair< sol::string_view, MouseButton > > mouseItems =
    {
            { "LEFT", MouseButton::LEFT },
            { "RIGHT", MouseButton::RIGHT },
            { "MIDDLE", MouseButton::MIDDLE },
    };
    lua.new_enum< MouseButton, false >( "MouseButton", mouseItems );
}