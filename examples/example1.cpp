#include "progression.h"
#include <iomanip>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Progression;
// using namespace std;

std::string rootDirectory;

float* genKernel(int size, float sigma) {
	float* kernel = new float[size];
	int halfSize = size / 2.0f;
	float sum = 0;
	for (int i = 0; i < size; i++) {
		kernel[i] = 1.0f / (std::sqrtf(2 * M_PI) * sigma)  * exp(-0.5 * pow((i - halfSize) / sigma, 2.0));
		sum += kernel[i];
	}
	for (int i = 0; i < size; i++) {
		kernel[i] /= sum;
		std::cout << std::setprecision(4) << kernel[i] << " ";
	}
	std::cout << std::endl << std::endl;

	return kernel;
}

int main(int argc, char* argv[]) {
	srand(time(NULL));

	rootDirectory = "C:/Users/Tyler/Documents/Progression/";


	auto& conf = PG::config::Config(rootDirectory + "configs/default.toml");
	if (!conf) {
		std::cout << "could not parse config file" << std::endl;
		exit(0);
	}

	PG::EngineInitialize(conf);

	std::cout << "\nbloom combine shader: " << std::endl;
	Shader bloomCombineShader = Shader(rootDirectory + "resources/shaders/bloomCombine.vert", rootDirectory + "resources/shaders/bloomCombine.frag");
	std::cout << "\nblur shader: " << std::endl;
	Shader blurShader = Shader(rootDirectory + "resources/shaders/blur.vert", rootDirectory + "resources/shaders/blur.frag");
	blurShader.AddUniform("kernel");
	std::cout << "\copy shader: " << std::endl;
	Shader copyShader = Shader(rootDirectory + "resources/shaders/copy.vert", rootDirectory + "resources/shaders/copy.frag");

	auto scene = Scene::Load(rootDirectory + "resources/scenes/glowSphereScene.pgscn");

	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));


	auto skybox = scene->getSkybox();


	GLuint postProcessFBO = graphics::CreateFrameBuffer();
	GLuint mainBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
	GLuint glowBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
	graphics::AttachColorTexturesToFBO({ mainBuffer, glowBuffer });
	GLuint rboDepth = graphics::CreateRenderBuffer(Window::getWindowSize().x, Window::getWindowSize().y);
	graphics::AttachRenderBufferToFBO(rboDepth);
	graphics::FinalizeFBO();

	GLuint pingPongFBO = graphics::CreateFrameBuffer();
	GLuint glowBufferBlur = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA16F, GL_LINEAR, GL_LINEAR);

	graphics::AttachColorTexturesToFBO({ glowBufferBlur });
	graphics::FinalizeFBO();

	float quadVerts[] = {
		-1, 1,
		-1, -1,
		1, -1,

		-1, 1,
		1, -1,
		1, 1
	};
	GLuint quadVAO;
	GLuint quadVBO;
	glGenVertexArrays(1, &quadVAO);
	glBindVertexArray(quadVAO);
	glGenBuffers(1, &quadVBO);
	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
	glEnableVertexAttribArray(bloomCombineShader["vertex"]);
	glVertexAttribPointer(bloomCombineShader["vertex"], 2, GL_FLOAT, GL_FALSE, 0, 0);

    // Note: After changing the input mode, should poll for events again
    Window::SetRelativeMouse(true);
    PG::Input::PollEvents();

	graphics::BindFrameBuffer(0);

	bool blur = true;
	bool bloom = true;
	auto kernel5 = genKernel(5, 3);
	auto kernel11 = genKernel(11, 3);
	auto kernel21 = genKernel(21, 5);
	auto kernel41 = genKernel(41, 10);
	float* kernels[] = { kernel5, kernel11, kernel21, kernel41 };
	int kernelSizes[] = { 5, 11, 21, 41 };

	const int levels = 4;
	const int divisorStart = 2;
	const int divisorMult  = 2;
	GLuint glowBuffers[levels][2];
	int divisor = divisorStart;
	for (int i = 0; i < levels; ++i) {
		glowBuffers[i][0] = graphics::Create2DTexture(Window::getWindowSize().x / divisor, Window::getWindowSize().y / divisor, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
		glowBuffers[i][1] = graphics::Create2DTexture(Window::getWindowSize().x / divisor, Window::getWindowSize().y / divisor, GL_RGBA16F, GL_LINEAR, GL_LINEAR);
		divisor *= divisorMult;
		std::cout << "level = " << i << std::endl;
	}

	// Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene->Update();

		graphics::BindFrameBuffer(postProcessFBO);

        RenderSystem::Render(scene);
		
		graphics::BindFrameBuffer(pingPongFBO);
		copyShader.Enable();
		glBindVertexArray(quadVAO);
		graphics::Bind2DTexture(glowBuffer, copyShader["tex"], 0);

		divisor = divisorStart;
		for (int i = 0; i < levels; ++i) {
			glViewport(0, 0, Window::getWindowSize().x / divisor, Window::getWindowSize().y / divisor);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glowBuffers[i][0], 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			divisor *= divisorMult;
		}

		// BLUR
		blurShader.Enable();
		glBindVertexArray(quadVAO);
		graphics::ToggleDepthBufferWriting(false);
		
		divisor = divisorStart;
		for (int i = 0; i < levels; ++i) {
			glUniform1fv(blurShader["kernel"], kernelSizes[i], kernels[i]);
			glUniform1i(blurShader["halfKernelWidth"], kernelSizes[i] / 2);

			glViewport(0, 0, Window::getWindowSize().x / divisor, Window::getWindowSize().y / divisor);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glowBuffers[i][1], 0);
			graphics::Bind2DTexture(glowBuffers[i][0], blurShader["tex"], 0);
			glUniform2f(blurShader["offset"], static_cast<float>(divisor) / Window::getWindowSize().x, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			graphics::Bind2DTexture(glowBuffers[i][1], blurShader["tex"], 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glowBuffers[i][0], 0);
			glUniform2f(blurShader["offset"], 0, static_cast<float>(divisor) / Window::getWindowSize().y);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			divisor *= divisorMult;
		}
		
		graphics::ToggleDepthBufferWriting(true);

		graphics::BindFrameBuffer(0);
		glViewport(0, 0, Window::getWindowSize().x, Window::getWindowSize().y);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		graphics::ToggleDepthBufferWriting(false);
		graphics::ToggleDepthTesting(false);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, postProcessFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, Window::getWindowSize().x, Window::getWindowSize().y, 0, 0, Window::getWindowSize().x, Window::getWindowSize().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		

		bloomCombineShader.Enable();
		glUniform1f(bloomCombineShader["bloomIntensity"], 1.0f);
		glUniform1f(bloomCombineShader["exposure"], 1);
		glBindVertexArray(quadVAO);
		graphics::Bind2DTexture(mainBuffer, bloomCombineShader["originalColor"], 0);
		graphics::Bind2DTexture(glowBuffers[0][0], bloomCombineShader["blur1"], 1);
		graphics::Bind2DTexture(glowBuffers[1][0], bloomCombineShader["blur2"], 2);
		graphics::Bind2DTexture(glowBuffers[2][0], bloomCombineShader["blur3"], 3);
		graphics::Bind2DTexture(glowBuffers[3][0], bloomCombineShader["blur4"], 4);
		glDrawArrays(GL_TRIANGLES, 0, 6);


		graphics::ToggleDepthTesting(true);
		graphics::ToggleDepthBufferWriting(true);
		

		skybox->Render(*camera);
		

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}