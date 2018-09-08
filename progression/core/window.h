#pragma once

#include "core/common.h"
#include "core/config.h"

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
        static nanogui::Screen* getUIScreen() { return _mUIScreen; }
        static glm::ivec2 getWindowSize() { return _mWindowSize; }

	protected:
        static GLFWwindow* _mWindow;
        static nanogui::Screen* _mUIScreen;

		static std::string _mTitle;
        static glm::ivec2 _mWindowSize;
	};

} // namespace Progression