#include "progression.h"

using namespace Progression;

std::string rootDirectory;

int main(int argc, char* argv[]) {
    srand(time(NULL));

    rootDirectory = "C:/Users/Tyler/Documents/Progression/";


    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::EngineInitialize(conf);

	std::cout << "\nbloom combine shader: " << std::endl;
	Shader bloomCombineShader = Shader(rootDirectory + "resources/shaders/bloomCombine.vert", rootDirectory + "resources/shaders/bloomCombine.frag");
	std::cout << "\nblur shader: " << std::endl;
	Shader blurShader         = Shader(rootDirectory + "resources/shaders/blur.vert", rootDirectory + "resources/shaders/blur.frag");

    Scene scene;

    Camera camera = Camera(Transform(
        glm::vec3(0, 0, 5),
        glm::vec3(0),
        glm::vec3(1)));
    camera.AddComponent<UserCameraComponent>(new UserCameraComponent(&camera));


    Light directionalLight(Light::Type::DIRECTIONAL);
    directionalLight.transform.rotation = glm::vec3(glm::radians(0.0f), glm::radians(0.0f), 0);

    // GameObject chalet(Transform(glm::vec3(0), glm::vec3(glm::radians(0.0f), 0, 0), glm::vec3(5)));
    GameObject gameObj(Transform(glm::vec3(0, 0, 0), glm::vec3(glm::radians(0.0f), glm::radians(0.0f), 0), glm::vec3(1)));
    
    auto model = ResourceManager::LoadModel("models/cube.obj", false);
    std::cout << "Model Info:" << std::endl;
    std::cout << "Num meshes: " << model->meshes.size() << std::endl;
    std::cout << "Num mats: " << model->materials.size() << std::endl;
    for (const auto& mesh : model->meshes) {
        static int i = 0;
        std::cout << "Mesh " << i << ": " << std::endl;
        std::cout << "\tNumVerts: " << mesh->numVertices << std::endl;
        std::cout << "\tNumTriangles: " << mesh->numTriangles << std::endl;
        i++;
    }
    model->materials[0]->diffuse = glm::vec3(.7, 0, 0);
	model->materials[0]->specular = glm::vec3(.7);
	model->materials[0]->emissive = glm::vec3(0, 0, .7);
	model->materials[0]->shininess = 60;

    gameObj.AddComponent<ModelRenderer>(new ModelRenderer(&gameObj, model.get()));
    auto modelRenderComponent = gameObj.GetComponent<ModelRenderer>();

    Light pointLight(Light::Type::POINT, Transform(gameObj.transform.position), glm::vec3(0, 0, 1), 1);

    GameObject floor(Transform(glm::vec3(0, -2, 0), glm::vec3(0), glm::vec3(20, 1, 20)));
    //floor.AddComponent<ModelRenderer>(new ModelRenderer(&floor, ResourceManager::GetModel("plane").get()));

    // scene.AddGameObject(&floor);
    scene.AddCamera(&camera);
    // scene.AddLight(&directionalLight);
    // scene.AddLight(&pointLight);
    scene.AddGameObject(&gameObj);

	
	GLuint postProcessFBO = graphics::CreateFrameBuffer();
	GLuint mainBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA);
	GLuint glowBuffer = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA);
	GLuint glowBufferBlur = graphics::Create2DTexture(Window::getWindowSize().x, Window::getWindowSize().y, GL_RGBA);

	graphics::AttachColorTexturesToFBO({ mainBuffer, glowBuffer });
	graphics::FinalizeFBO();
	
	/*
	unsigned int postProcessFBO;
	glGenFramebuffers(1, &postProcessFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, postProcessFBO);
	unsigned int colorBuffers[2];
	glGenTextures(2, colorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGB16F, Window::getWindowSize().x, Window::getWindowSize().y, 0, GL_RGB, GL_FLOAT, NULL
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		// attach texture to framebuffer
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0
		);
	}
	*/

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

    glEnable(GL_DEPTH_TEST);

	graphics::BindFrameBuffer(0);

    // Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene.Update();

		graphics::BindFrameBuffer(postProcessFBO);

        RenderSystem::Render(&scene);

		
		blurShader.Enable();

		glBindVertexArray(quadVAO);

		
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glowBufferBlur, 0);
		graphics::Bind2DTexture(glowBuffer, blurShader["tex"], 0);
		glUniform2f(blurShader["offset"], 1.2f / Window::getWindowSize().x, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);

		graphics::Bind2DTexture(glowBufferBlur, blurShader["tex"], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, glowBuffer, 0);
		glUniform2f(blurShader["offset"], 0, 1.2f / Window::getWindowSize().y);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		

		graphics::BindFrameBuffer(0);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		bloomCombineShader.Enable();
		glBindVertexArray(quadVAO);
		graphics::Bind2DTexture(mainBuffer, bloomCombineShader["originalColor"], 0);
		graphics::Bind2DTexture(glowBuffer, bloomCombineShader["glow0"], 1);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		
		/*
		graphics::BindFrameBuffer(0);

		bloomCombineShader.Enable();
		graphics::ToggleDepthTesting(true);
		glBindVertexArray(quadVAO);
		graphics::Bind2DTexture(colorBuffers[0], bloomCombineShader["originalColor"], 0);
		graphics::Bind2DTexture(colorBuffers[1], bloomCombineShader["glow0"], 1);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		*/
		

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}