#include "core/window.hpp"
#include "core/time.hpp"
#include "utils/logger.hpp"
#include <unordered_set>

static std::unordered_set< size_t > s_debugMessages;

static std::chrono::high_resolution_clock::time_point s_lastFPSUpdateTime;
static unsigned int s_framesDrawnSinceLastFPSUpdate = 0;

static void ErrorCallback( int err, const char* description )
{
    PG_MAYBE_UNUSED( err );
    PG_MAYBE_UNUSED( description );
    LOG_ERR( "GLFW ERROR: ", err, ", ", description );
}

static Progression::Window* s_mainWindow = nullptr;

namespace Progression
{
void InitWindowSystem( const WindowCreateInfo& info )
{
    if ( !glfwInit() )
    {
        LOG_ERR( "Could not initialize GLFW" );
        exit( EXIT_FAILURE );
    }

    s_mainWindow = new Window;
    s_mainWindow->Init( info );

    s_mainWindow->BindContext();
}

void ShutdownWindowSystem()
{
    delete s_mainWindow;
    glfwTerminate();
}

Window* GetMainWindow()
{
    return s_mainWindow;
}

Window::~Window()
{
    if ( m_window )
    {
        glfwDestroyWindow( m_window );
        m_window = nullptr;
    }
}

void Window::Init( const WindowCreateInfo& createInfo )
{
    m_title   = createInfo.title;
    m_width   = createInfo.width;
    m_height  = createInfo.height;
    m_visible = createInfo.visible;

    // default non-configurable settings
    glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
    glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
    glfwWindowHint( GLFW_SAMPLES, 0 );
    glfwWindowHint( GLFW_RED_BITS, 8 );
    glfwWindowHint( GLFW_GREEN_BITS, 8 );
    glfwWindowHint( GLFW_BLUE_BITS, 8 );
    glfwWindowHint( GLFW_ALPHA_BITS, 8 );
    glfwWindowHint( GLFW_STENCIL_BITS, 8 );
    glfwWindowHint( GLFW_DEPTH_BITS, 24 );

    glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
    glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 3 );
    glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );
    glfwWindowHint( GLFW_OPENGL_DEBUG_CONTEXT, createInfo.debugContext );

    glfwWindowHint( GLFW_VISIBLE, m_visible );
    GLFWwindow* parentWindow = GetMainWindow() == this ? NULL : GetMainWindow()->GetGLFWHandle();
    m_window = glfwCreateWindow( m_width, m_height, m_title.c_str(), NULL, parentWindow );
    if ( !m_window )
    {
        LOG_ERR( "Window or context creation failed" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    if ( parentWindow )
    {
        return;
    }

    BindContext();

    glfwSetErrorCallback( ErrorCallback );

    if ( createInfo.vsync )
    {
        glfwSwapInterval( 1 );
    }
    else
    {
        glfwSwapInterval( 0 );
    }

#if !USING( SHIP_BUILD )
    if ( createInfo.debugContext )
    {
        
    }
#endif // #if !USING( SHIP_BUILD )

    s_framesDrawnSinceLastFPSUpdate = 0;
    s_lastFPSUpdateTime             = Time::GetTimePoint();
}

void Window::BindContext()
{
    // glfwMakeContextCurrent( m_window );
}

void Window::UnbindContext()
{
    // glfwMakeContextCurrent( NULL );
}

void Window::SwapWindow()
{
    glfwSwapBuffers( m_window );
}

void Window::StartFrame()
{
    Time::StartFrame();
}

void Window::EndFrame()
{
    Time::EndFrame();
    SwapWindow();
    ++s_framesDrawnSinceLastFPSUpdate;
    if ( Time::GetDuration( s_lastFPSUpdateTime ) > 1000.0f )
    {
        std::string titleWithFps =
          m_title + " -- FPS: " + std::to_string( s_framesDrawnSinceLastFPSUpdate );
        glfwSetWindowTitle( m_window, titleWithFps.c_str() );
        s_framesDrawnSinceLastFPSUpdate = 0;
        s_lastFPSUpdateTime             = Time::GetTimePoint();
    }
}

void Window::SetRelativeMouse( bool b )
{
    if ( b )
    {
        glfwSetInputMode( m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
    }
    else
    {
        glfwSetInputMode( m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
    }
}

void Window::SetTitle( const std::string& title )
{
    m_title = title;
    glfwSetWindowTitle( m_window, m_title.c_str() );
}


} // namespace Progression
