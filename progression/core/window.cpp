#include "core/window.hpp"
#include "core/time.hpp"
#include "glad/glad.h"
#include "utils/logger.hpp"
#include <unordered_set>

static std::unordered_set< size_t > s_debugMessages;

static std::chrono::high_resolution_clock::time_point s_lastFPSUpdateTime;
static unsigned int s_framesDrawnSinceLastFPSUpdate = 0;

// static s_numFramesSince

// debug context log function just copied from learnopengl.com
static void APIENTRY GLDebugOutput( GLenum source,
                                    GLenum type,
                                    GLuint id,
                                    GLenum severity,
                                    GLsizei length,
                                    const GLchar* message,
                                    const void* userParam )
{
    PG_UNUSED( length );
    PG_UNUSED( userParam );

    // ignore non-significant error/warning codes
    if ( id == 131169 || id == 131185 || id == 131218 || id == 131204 )
    {
        return;
    }

    std::hash< std::string > hasher;
    auto hash = hasher( std::string( message ) );
    if ( s_debugMessages.find( hash ) != s_debugMessages.end() )
    {
        return;
    }
    else
    {
        s_debugMessages.insert( hash );
    }


    LOG_WARN( "---------------" );
    LOG_WARN( "Debug message (", id, "): ", message );

    std::string msg = "";
    switch ( source )
    {
        case GL_DEBUG_SOURCE_API:
            LOG_WARN( "Source: API" );
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            LOG_WARN( "Source: Window System" );
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            LOG_WARN( "Source: Shader Compiler" );
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            LOG_WARN( "Source: Third Party" );
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            LOG_WARN( "Source: Application" );
            break;
        case GL_DEBUG_SOURCE_OTHER:
            LOG_WARN( "Source: Other" );
            break;
    }

    switch ( type )
    {
        case GL_DEBUG_TYPE_ERROR:
            LOG_WARN( "Type: Error" );
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            LOG_WARN( "Type: Deprecated Behaviour" );
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            LOG_WARN( "Type: Undefined Behaviour" );
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            LOG_WARN( "Type: Portability" );
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            LOG_WARN( "Type: Performance" );
            break;
        case GL_DEBUG_TYPE_MARKER:
            LOG_WARN( "Type: Marker" );
            break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            LOG_WARN( "Type: Push Group" );
            break;
        case GL_DEBUG_TYPE_POP_GROUP:
            LOG_WARN( "Type: Pop Group" );
            break;
        case GL_DEBUG_TYPE_OTHER:
            LOG_WARN( "Type: Other" );
            break;
    }

    switch ( severity )
    {
        case GL_DEBUG_SEVERITY_HIGH:
            LOG_WARN( "Severity: high" );
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            LOG_WARN( "Severity: medium" );
            break;
        case GL_DEBUG_SEVERITY_LOW:
            LOG_WARN( "Severity: low" );
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            LOG_WARN( "Severity: notification" );
            break;
    }
    LOG( "" );
}

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

    if ( !gladLoadGLLoader( (GLADloadproc)glfwGetProcAddress ) )
    {
        LOG_ERR( "Failed to gladLoadGLLoader" );
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    if ( GetMainWindow() == this )
    {
        LOG( "Vendor: ", glGetString( GL_VENDOR ) );
        LOG( "Renderer: ", glGetString( GL_RENDERER ) );
        LOG( "Version: ", glGetString( GL_VERSION ) );
    }

    if ( createInfo.vsync )
    {
        glfwSwapInterval( 1 );
    }
    else
    {
        glfwSwapInterval( 0 );
    }

    int fbW, fbH, wW, wH;
    glfwGetFramebufferSize( m_window, &fbW, &fbH );
    glfwGetWindowSize( m_window, &wW, &wH );

    glViewport( 0, 0, fbW, fbH );

#if !USING( SHIP_BUILD )
    if ( createInfo.debugContext )
    {
        GLint flags;
        glGetIntegerv( GL_CONTEXT_FLAGS, &flags );
        if ( flags & GL_CONTEXT_FLAG_DEBUG_BIT )
        {
            LOG( "enabling debug context" );
            glEnable( GL_DEBUG_OUTPUT );
            glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS );
            glDebugMessageCallback( GLDebugOutput, nullptr );
            glDebugMessageControl( GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE );
        }
    }
#endif // #if !USING( SHIP_BUILD )

    s_framesDrawnSinceLastFPSUpdate = 0;
    s_lastFPSUpdateTime             = Time::GetTimePoint();
}

void Window::BindContext()
{
    glfwMakeContextCurrent( m_window );
}

void Window::UnbindContext()
{
    glfwMakeContextCurrent( NULL );
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
