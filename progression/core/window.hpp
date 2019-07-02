#pragma once

#include "core/common.hpp"
#include "core/config.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

    struct WindowCreateInfo {
        std::string title = "Untitled";
        int width         = 1280;
        int height        = 720;
        bool visible      = true;
        bool debugContext = true;
        bool vsync        = false;
    };

	class Window : public NonCopyable {
	public:
        Window() = default;
        ~Window();

        void init(const struct WindowCreateInfo& createInfo);
        void shutdown();
        void swapWindow();
		void startFrame();
		void endFrame();

        GLFWwindow* getGLFWHandle() const { return window_; }
        int width() const { return width_; }
        int height() const { return height_; }
        void setRelativeMouse(bool b);
		void setTitle(const std::string& title);
        void bindContext();
        void unbindContext();

	protected:
        GLFWwindow* window_ = nullptr;
		std::string title_  = "";
        int width_          = 0;
        int height_         = 0;
        bool visible_       = false;
	};

    void initWindowSystem(const WindowCreateInfo& info);
    void shutdownWindowSystem();
    Window* getMainWindow();

} // namespace Progression
