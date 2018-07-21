#include "include/window.h"

namespace Progression {

	Window::Window(const std::string& title, int w, int h) :
		title_(title),
		width_(w),
		height_(h),
		fpsCounter_()
	{
		Init();
	}

	Window::~Window() {
		SDL_GL_DeleteContext(glContext_);
		SDL_DestroyWindow(sdlWindow_);
	}

	void Window::Init() {
		SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);

		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
		sdlWindow_ = SDL_CreateWindow(
			title_.c_str(),
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			width_,
			height_,
			SDL_WINDOW_OPENGL);

		if (sdlWindow_ == NULL) {
			std::cout << "Failed to create an SDL2 window" << std::endl;
			exit(EXIT_FAILURE);
		}
		glContext_ = SDL_GL_CreateContext(sdlWindow_);
		if (glContext_ == NULL) {
			std::cout << "Failed to create an opengl context" << std::endl;
			exit(EXIT_FAILURE);
		}

		/*glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK) {
			std::cerr << "Failed to init GLEW" << std::endl;
			exit(EXIT_FAILURE);
		}*/
		if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
			std::cout << "Failed to gladLoadGLLoader" << std::endl;
			exit(EXIT_FAILURE);
		}
		
		std::cout << "Vendor: " << glGetString(GL_VENDOR) << std::endl;
		std::cout << "Renderer: " << glGetString(GL_RENDERER) << std::endl;
		std::cout << "Version: " << glGetString(GL_VERSION) << std::endl;

		/*
		if (SDL_GL_SetSwapInterval(1) < 0)
			std::cerr << "Failed to set vsync" << std::endl;
		*/

		glEnable(GL_DEPTH_TEST);
	}

	void Window::SwapWindow() {
		SDL_GL_SwapWindow(sdlWindow_);
	}

	void Window::SetRelativeMouse(bool b) {
		if (b)
			SDL_SetRelativeMouseMode(SDL_TRUE);
		else
			SDL_SetRelativeMouseMode(SDL_FALSE);
	}

	float Window::GetTotalRuntime() const {
		return SDL_GetTicks() / 1000.0f;
	}

	float Window::GetDT() const {
		return fpsCounter_.GetDT();
	}

	void Window::StartFrame() {
		fpsCounter_.StartFrame(GetTotalRuntime());
	}

	void Window::EndFrame() {
		fpsCounter_.EndFrame();
		SwapWindow();
	}

} // namespace Progression