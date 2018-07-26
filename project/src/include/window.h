#pragma once

#include "include/utils.h"

namespace Progression {

	class Window {

		class FPSCounter {
		public:
			FPSCounter() : display_(true), time_(0), prevTime_(0), fpsTime_(0), frameCounter_(0) {}
			~FPSCounter() = default;

			void StartFrame(float t) { time_ = t; }

			void EndFrame() {
				prevTime_ = time_;
				++frameCounter_;
				if (time_ > fpsTime_ + 1) {
					if (display_)
						std::cout << "FPS: " << frameCounter_ << std::endl;
					frameCounter_ = 0;
					fpsTime_ = time_;
				}
			}

			float GetDT() const { return time_ - prevTime_; }
			void Display(bool b) { display_ = b; }

		private:
			bool display_;
			float time_;
			float prevTime_;
			float fpsTime_;
			unsigned int frameCounter_;
		};

	public:
		Window(const std::string& title = "Untitled", int w = 640, int h = 480, bool vsync = true);
		~Window();
		void Init();
		void SwapWindow();
		void StartFrame();
		void EndFrame();
		float GetTotalRuntime() const;
		float GetDT() const;

		void SetRelativeMouse(bool b);

        GLFWwindow* getWindow() const { return window_; }
		void showFPS(bool b) { fpsCounter_.Display(b); }
        int getViewportWidth() const { return viewportWidth_; }
        int getViewportHeight() const { return viewportHeight_; }

	protected:
		std::string title_;
		int screenWidth_;
		int screenHeight_;
        int viewportWidth_;
        int viewportHeight_;
        FPSCounter fpsCounter_;
        GLFWwindow* window_;
        bool vsync_;
	};

} // namespace Progression