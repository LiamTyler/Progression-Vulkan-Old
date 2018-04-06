#pragma once

#include "include/utils.h"
#include "include/fps_counter.h"

class Window {
    public:
        Window();
        Window(const std::string& title, int w, int h);
        ~Window();
        void Init();
        void SwapWindow();
        void StartFrame();
        void EndFrame();
        float GetTotalRuntime();
        float GetDT();

        void SetRelativeMouse(bool b);

        SDL_Window* GetWindow() { return sdlWindow_; }
        SDL_GLContext GetContext() { return glContext_; }
        void ShowFPS(bool b) { fpsCounter_.Display(b); }

    protected:
        std::string title_;
        int width_;
        int height_;
        bool show_fps_;
        FPSCounter fpsCounter_;

        // SDL specifics
        SDL_Window* sdlWindow_;
        SDL_GLContext glContext_;
};
