#pragma once

#include "core/common.hpp"
#include "core/config.hpp"

namespace Progression {

	class Window {
	public:
        Window() = delete;
        ~Window() = delete;

        static void Init(const config::Config& config);
        static void Free();
        static void SwapWindow();
		static void StartFrame();
		static void EndFrame();

        static GLFWwindow* getGLFWHandle() { return _mWindow; }
        static int width() { return _mWindowSize.x; }
        static int height() { return _mWindowSize.y; }
        static void SetRelativeMouse(bool b);
		static void setTitle(const std::string& title);

	protected:
        static GLFWwindow* _mWindow;

		static std::string _mTitle;
        static glm::ivec2 _mWindowSize;
	};

} // namespace Progression
