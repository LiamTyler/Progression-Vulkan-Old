#pragma once

#include "core/common.hpp"
#include "utils/noncopyable.hpp"

namespace Progression
{

struct WindowCreateInfo
{
    std::string title = "Untitled";
    int width         = 1280;
    int height        = 720;
    bool visible      = true;
    bool debugContext = true;
    bool vsync        = false;
};

class Window : public NonCopyable
{
public:
    Window() = default;
    ~Window();

    void Init( const struct WindowCreateInfo& createInfo );
    void Shutdown();
    void SwapWindow();
    void StartFrame();
    void EndFrame();

    GLFWwindow* GetGLFWHandle() const { return m_window; }
    int Width() const { return m_width; }
    int Height() const { return m_height; }
    void SetRelativeMouse( bool b );
    void SetTitle( const std::string& title );
    void BindContext();
    void UnbindContext();

protected:
    GLFWwindow* m_window = nullptr;
    std::string m_title  = "";
    int m_width          = 0;
    int m_height         = 0;
    bool m_visible       = false;
};

void InitWindowSystem( const WindowCreateInfo& info );
void ShutdownWindowSystem();
Window* GetMainWindow();

} // namespace Progression
