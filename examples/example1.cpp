#include "progression.h"

using namespace Progression;

std::string rootDirectory;

int main(int argc, char* argv[]) {
    srand(time(NULL));

    rootDirectory = "C:/Users/Liam Tyler/Documents/Progression/";


    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::EngineInitialize(conf);

	std::cout << "\nbloom combine shader: " << std::endl;
	Shader bloomCombineShader = Shader(rootDirectory + "resources/shaders/bloomCombine.vert", rootDirectory + "resources/shaders/bloomCombine.frag");
	std::cout << "\nblur shader: " << std::endl;
	Shader blurShader         = Shader(rootDirectory + "resources/shaders/blur.vert", rootDirectory + "resources/shaders/blur.frag");
	std::cout << "\copy shader: " << std::endl;
	Shader copyShader = Shader(rootDirectory + "resources/shaders/copy.vert", rootDirectory + "resources/shaders/copy.frag");

	auto scene = Scene::Load(rootDirectory + "resources/scenes/glowSphereScene.pgscn");

	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 15));


	auto skybox = scene->getSkybox();

	
	GLuint postProcessFBO = graphics::CreateFrameBuffer();
	GLuint mainBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA, GL_LINEAR, GL_LINEAR);
	GLuint glowBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA, GL_LINEAR, GL_LINEAR);
	graphics::AttachColorTexturesToFBO({ mainBuffer, glowBuffer });
	GLuint rboDepth = graphics::CreateRenderBuffer(Window::getWindowSize().x, Window::getWindowSize().y);
	graphics::AttachRenderBufferToFBO(rboDepth);
	graphics::FinalizeFBO();

	GLuint pingPongFBO = graphics::CreateFrameBuffer();
	GLuint glowBufferBlur = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA, GL_LINEAR, GL_LINEAR);

	graphics::AttachColorTexturesToFBO({ glowBufferBlur });
	graphics::FinalizeFBO();

	GLuint halfGlowBuffer = graphics::Create2DTexture(Window::getWindowSize().x / 2, Window::getWindowSize().y / 2, GL_RGBA, GL_LINEAR, GL_LINEAR);
	GLuint quarterGlowBuffer = graphics::Create2DTexture(Window::getWindowSize().x / 4, Window::getWindowSize().y / 4, GL_RGBA, GL_LINEAR, GL_LINEAR);
	GLuint eigthGlowBuffer = graphics::Create2DTexture(Window::getWindowSize().x / 8, Window::getWindowSize().y / 8, GL_RGBA, GL_LINEAR, GL_LINEAR);
	GLuint sixteenthGlowBuffer = graphics::Create2DTexture(Window::getWindowSize().x / 16, Window::getWindowSize().y / 16, GL_RGBA, GL_LINEAR, GL_LINEAR);

	GLuint halfGlowBuffer2 = graphics::Create2DTexture(Window::getWindowSize().x / 2, Window::getWindowSize().y / 2, GL_RGBA, GL_LINEAR, GL_LINEAR);
	GLuint quarterGlowBuffer2 = graphics::Create2DTexture(Window::getWindowSize().x / 4, Window::getWindowSize().y / 4, GL_RGBA, GL_LINEAR, GL_LINEAR);
	GLuint eigthGlowBuffer2 = graphics::Create2DTexture(Window::getWindowSize().x / 8, Window::getWindowSize().y / 8, GL_RGBA, GL_LINEAR, GL_LINEAR);
	GLuint sixteenthGlowBuffer2 = graphics::Create2DTexture(Window::getWindowSize().x / 16, Window::getWindowSize().y / 16, GL_RGBA, GL_LINEAR, GL_LINEAR);

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
	// Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;
		if (PG::Input::GetKeyDown(PG::PG_K_G))
			blur = !blur;

		if (PG::Input::GetKeyDown(PG::PG_K_B))
			bloom = !bloom;

        scene->Update();

		graphics::BindFrameBuffer(postProcessFBO);

        RenderSystem::Render(scene);
		
		graphics::BindFrameBuffer(pingPongFBO);
		copyShader.Enable();
		glBindVertexArray(quadVAO);
		graphics::Bind2DTexture(glowBuffer, copyShader["tex"], 0);

		glViewport(0, 0, Window::getWindowSize().x / 2, Window::getWindowSize().y / 2);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, halfGlowBuffer, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glViewport(0, 0, Window::getWindowSize().x / 4, Window::getWindowSize().y / 4);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, quarterGlowBuffer, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glViewport(0, 0, Window::getWindowSize().x / 8, Window::getWindowSize().y / 8);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, eigthGlowBuffer, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		glViewport(0, 0, Window::getWindowSize().x / 16, Window::getWindowSize().y / 16);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sixteenthGlowBuffer, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// BLUR
		blurShader.Enable();
		glBindVertexArray(quadVAO);
		
		if (blur) {
			// Blur half resolution glowBuffer
			//---------------------------------------------------------------------------------------------
			glViewport(0, 0, Window::getWindowSize().x / 2, Window::getWindowSize().y / 2);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, halfGlowBuffer2, 0);
			graphics::Bind2DTexture(halfGlowBuffer, blurShader["tex"], 0);
			glUniform2f(blurShader["offset"], 1.0f / Window::getWindowSize().x * 2.0f, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			graphics::Bind2DTexture(halfGlowBuffer2, blurShader["tex"], 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, halfGlowBuffer, 0);
			glUniform2f(blurShader["offset"], 0, 1.0f / Window::getWindowSize().y * 2.0f);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			// Blur quarter resolution glowBuffer
			//---------------------------------------------------------------------------------------------
			glViewport(0, 0, Window::getWindowSize().x / 4, Window::getWindowSize().y / 4);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, quarterGlowBuffer2, 0);
			graphics::Bind2DTexture(quarterGlowBuffer, blurShader["tex"], 0);
			glUniform2f(blurShader["offset"], 1.0f / Window::getWindowSize().x * 4.0f, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			graphics::Bind2DTexture(quarterGlowBuffer2, blurShader["tex"], 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, quarterGlowBuffer, 0);
			glUniform2f(blurShader["offset"], 0, 1.0f / Window::getWindowSize().y * 4.0f);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			// Blur eigth resolution glowBuffer
			//---------------------------------------------------------------------------------------------
			glViewport(0, 0, Window::getWindowSize().x / 8, Window::getWindowSize().y / 8);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, eigthGlowBuffer2, 0);
			graphics::Bind2DTexture(eigthGlowBuffer, blurShader["tex"], 0);
			glUniform2f(blurShader["offset"], 1.0f / Window::getWindowSize().x * 8.0f, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			graphics::Bind2DTexture(eigthGlowBuffer2, blurShader["tex"], 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, eigthGlowBuffer, 0);
			glUniform2f(blurShader["offset"], 0, 1.0f / Window::getWindowSize().y * 8.0f);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			glViewport(0, 0, Window::getWindowSize().x / 16, Window::getWindowSize().y / 16);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sixteenthGlowBuffer2, 0);
			graphics::Bind2DTexture(sixteenthGlowBuffer, blurShader["tex"], 0);
			glUniform2f(blurShader["offset"], 1.0f / Window::getWindowSize().x * 16.0f, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);

			graphics::Bind2DTexture(sixteenthGlowBuffer2, blurShader["tex"], 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sixteenthGlowBuffer, 0);
			glUniform2f(blurShader["offset"], 0, 1.0f / Window::getWindowSize().y * 16.0f);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}
		

		graphics::BindFrameBuffer(0);
		glViewport(0, 0, Window::getWindowSize().x, Window::getWindowSize().y);

		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		graphics::ToggleDepthBufferWriting(false);
		graphics::ToggleDepthTesting(false);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, postProcessFBO);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, Window::getWindowSize().x, Window::getWindowSize().y, 0, 0, Window::getWindowSize().x, Window::getWindowSize().y, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		
		if (!bloom) {
			copyShader.Enable();
			glBindVertexArray(quadVAO);
			graphics::Bind2DTexture(mainBuffer, copyShader["tex"], 0);

			glDrawArrays(GL_TRIANGLES, 0, 6);

		} else {
			bloomCombineShader.Enable();
			glBindVertexArray(quadVAO);
			graphics::Bind2DTexture(mainBuffer, bloomCombineShader["originalColor"], 0);
			graphics::Bind2DTexture(sixteenthGlowBuffer, bloomCombineShader["fullGlow"], 1);
			graphics::Bind2DTexture(halfGlowBuffer, bloomCombineShader["halfGlow"], 2);
			graphics::Bind2DTexture(quarterGlowBuffer, bloomCombineShader["quarterGlow"], 3);
			graphics::Bind2DTexture(eigthGlowBuffer, bloomCombineShader["eigthGlow"], 4);
			glDrawArrays(GL_TRIANGLES, 0, 6);
		}

		graphics::ToggleDepthTesting(true);
		graphics::ToggleDepthBufferWriting(true);
		

		skybox->Render(*camera);
		

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}