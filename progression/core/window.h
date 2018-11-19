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
        static glm::ivec2 getWindowSize() { return _mWindowSize; }
        static void SetRelativeMouse(bool b);

	protected:
        static GLFWwindow* _mWindow;

		static std::string _mTitle;
        static glm::ivec2 _mWindowSize;
	};

} // namespace Progression