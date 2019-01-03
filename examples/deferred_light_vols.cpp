#include "progression.h"

using namespace Progression;

std::string rootDirectory;

class LightBallComponent : public Component {
public:
	LightBallComponent(GameObject* obj, GameObject* _ball) :
		Component(obj),
		ball(_ball)
	{
	}

	~LightBallComponent() = default;

	void Start() {
	}

	void Update() {
		gameObject->transform = ball->transform;
	}

	void Stop() {

	}

	GameObject* ball;
};

class BounceComponent : public Component {
public:
	BounceComponent(GameObject* obj, const glm::vec3 startVel = glm::vec3(0, 0, 0)) :
		Component(obj),
		velocity(startVel)
	{
	}

	~BounceComponent() = default;

	void Start() {
	}

	void Update() {
		float dt = 1.0f / 30.0f;
		//float dt = Time::deltaTime();
		velocity.y += -9.81f * dt;
		gameObject->transform.position += velocity * dt;
		if (gameObject->transform.position.y < gameObject->transform.scale.x) {
			gameObject->transform.position.y = gameObject->transform.scale.x;
			velocity.y *= -.97;
		}
	}

	void Stop() {

	}

	glm::vec3 velocity;
};

// argv[1] = path of the root directory
int main(int argc, char* argv[]) {
	srand(time(NULL));

	auto conf = PG::config::Config(PG_ROOT_DIR "configs/bouncing_ball.toml");

	PG::EngineInitialize(conf);

	Shader deferredShader = Shader(PG_RESOURCE_DIR "shaders/deferred_phong.vert", PG_RESOURCE_DIR "shaders/deferred_phong.frag");
	Shader combineShader = Shader(PG_RESOURCE_DIR "shaders/deferred_combine2.vert", PG_RESOURCE_DIR "shaders/deferred_combine2.frag");
	Shader lightVolumeShader = Shader(PG_RESOURCE_DIR "shaders/lightVolume.vert", PG_RESOURCE_DIR "shaders/lightVolume.frag");

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
	glEnableVertexAttribArray(combineShader["vertex"]);
	glVertexAttribPointer(combineShader["vertex"], 2, GL_FLOAT, GL_FALSE, 0, 0);

	auto scene = Scene::Load(PG_RESOURCE_DIR "scenes/bouncing_ball.pgscn");

	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 15));

	auto ballModel = ResourceManager::GetModel("metalBall");
	auto planeModel = ResourceManager::GetModel("metalFloor");

	int X, Z, startX, startZ;
	float DX, DZ;

	int numBalls;
	auto val = conf->get_table("app");
	numBalls = *val->get_as<int>("numBalls");

	if (numBalls == 9) {
		X = 3;
		Z = 3;
		startX = 0;
		startZ = 0;
		DX = 1;
		DZ = 1;
	}
	else if (numBalls == 100) {
		X = 10;
		Z = 10;
		startX = 0;
		startZ = 0;
		DX = 1;
		DZ = 1;
	}
	else if (numBalls == 400) {
		X = 20;
		Z = 20;
		startX = -X;
		startZ = -Z;
		DX = 1;
		DZ = 1;
	}
	else if (numBalls == 1000) {
		X = 32;
		Z = 32;
		startX = -X;
		startZ = -Z;
		DX = 1;
		DZ = 1;
	}
	else if (numBalls == 4000) {
		X = 64;
		Z = 64;
		startX = -X;
		startZ = -Z;
		DX = 1;
		DZ = 1;
	}
	else if (numBalls == 10000) {
		X = 100;
		Z = 100;
		startX = -X;
		startZ = -Z;
		DX = 1;
		DZ = 1;
	}

	float intensity = *val->get_as<float>("intensity");
	float cutOffIntensity = 0.03;
	for (float x = 0; x < X; x += DX) {
		for (float z = 0; z < Z; z += DZ) {
			float randHeight = 3 + 2 * (rand() / static_cast<float>(RAND_MAX));
			glm::vec3 pos = glm::vec3(startX + 2 * x, randHeight, startZ + 2 * z);
			GameObject* ballObj = new GameObject(Transform(pos, glm::vec3(0), glm::vec3(.5)));
			ballObj->AddComponent<ModelRenderer>(new ModelRenderer(ballObj, ballModel.get()));
			ballObj->AddComponent<BounceComponent>(new BounceComponent(ballObj));

			glm::vec3 randColor = glm::vec3((rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)), (rand() / static_cast<float>(RAND_MAX)));
			PG::Light* pl = new Light(PG::Light::Type::POINT, pos, randColor, intensity);
			pl->AddComponent<LightBallComponent>(new LightBallComponent(pl, ballObj));
			float lightRadius = sqrtf(intensity / cutOffIntensity);
			// lightRadius = 4;

			scene->AddGameObject(ballObj);
			scene->AddLight(pl);
		}
	}
	std::cout << "light Radius = " << sqrtf(intensity / cutOffIntensity) << std::endl;


	GameObject* planeObj = scene->GetGameObject("floor");
	planeObj->AddComponent<ModelRenderer>(new ModelRenderer(planeObj, planeModel.get()));

	auto skybox = scene->getSkybox();


	unsigned int gBuffer;
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	unsigned int gPosition, gNormal, gDiffuse, gSpecularExp;

	// - position color buffer 
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, Window::width(), Window::height(), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

	// - normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, Window::width(), Window::height(), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);

	// - diffuse color buffer
	glGenTextures(1, &gDiffuse);
	glBindTexture(GL_TEXTURE_2D, gDiffuse);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Window::width(), Window::height(), 0, GL_RGB, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gDiffuse, 0);

	// - specular color + specular exp buffer
	glGenTextures(1, &gSpecularExp);
	glBindTexture(GL_TEXTURE_2D, gSpecularExp);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, Window::width(), Window::height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gSpecularExp, 0);


	// - tell OpenGL which color attachments we'll use (of this framebuffer) for rendering 
	unsigned int attachments[4] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };
	glDrawBuffers(4, attachments);

	unsigned int rboDepth;
	glGenRenderbuffers(1, &rboDepth);
	glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, Window::width(), Window::height());
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "frame buffer incomplete" << std::endl;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	auto lightVolumeVAO = graphics::CreateVAO();
	GLuint* lightVolumeVBOs = &ballModel->meshes[0]->vbos[0];
	glBindBuffer(GL_ARRAY_BUFFER, lightVolumeVBOs[Mesh::vboName::VERTEX]);
	glEnableVertexAttribArray(lightVolumeShader["vertex"]);
	glVertexAttribPointer(lightVolumeShader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightVolumeVBOs[Mesh::vboName::INDEX]);

	// Note: After changing the input mode, should poll for events again
	Window::SetRelativeMouse(true);
	PG::Input::PollEvents();

	glEnable(GL_DEPTH_TEST);

	while (!PG::Input::GetKeyDown(PG::PG_K_P))
		PG::Input::PollEvents();

	// Game loop
	while (!PG::EngineShutdown) {
		PG::Window::StartFrame();
		PG::Input::PollEvents();

		if (PG::Input::GetKeyDown(PG::PG_K_ESC))
			PG::EngineShutdown = true;

		scene->Update();

		//RenderSystem::Render(scene);


		RenderSystem::UpdateLights(scene, camera);


		glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);

		//const auto& bgColor = scene.GetBackgroundColor();
		// glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		deferredShader.Enable();

		RenderSystem::UploadCameraProjection(deferredShader, *camera);

		const auto& gameObjects = scene->GetGameObjects();
		// std::cout << gameObjects.size() << std::endl;
		for (const auto& obj : scene->GetGameObjects()) {
			glm::mat4 M = obj->transform.GetModelMatrix();
			glm::mat4 MV = camera->GetV() * M;
			glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
			glUniformMatrix4fv(deferredShader["modelViewMatrix"], 1, GL_FALSE, glm::value_ptr(MV));
			glUniformMatrix4fv(deferredShader["normalMatrix"], 1, GL_FALSE, glm::value_ptr(normalMatrix));
			for (const auto& mr : obj->GetComponent<ModelRenderer>()->meshRenderers) {
				glBindVertexArray(mr->vao);
				RenderSystem::UploadMaterial(deferredShader, *mr->material);
				glDrawElements(GL_TRIANGLES, mr->mesh->numTriangles * 3, GL_UNSIGNED_INT, 0);
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glViewport(0, 0, Window::width(), Window::height());

		glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		glBlitFramebuffer(0, 0, Window::width(), Window::height(), 0, 0, Window::width(), Window::height(), GL_DEPTH_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		lightVolumeShader.Enable();

		// Turn on additive blending and disable depth writing
		graphics::ToggleBlending(true);
		graphics::ToggleCulling(true);
		graphics::ToggleDepthBufferWriting(false);
		graphics::ToggleDepthTesting(false);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, gPosition);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, gNormal);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gDiffuse);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gSpecularExp);
		glUniform1i(lightVolumeShader["gPosition"], 0);
		glUniform1i(lightVolumeShader["gNormal"], 1);
		glUniform1i(lightVolumeShader["gDiffuse"], 2);
		glUniform1i(lightVolumeShader["gSpecularExp"], 3);

		glBindVertexArray(lightVolumeVAO);
		const auto& pointLights = scene->GetPointLights();
		for (const auto& light : pointLights) {

			// calculate light volume model matrix
			float cutOffIntensity = 0.003;
			float lightRadius = sqrtf(light->intensity / cutOffIntensity);
			//lightRadius = 4;
			glm::mat4 lightModelMatrix(1);
			lightModelMatrix = glm::translate(lightModelMatrix, light->transform.position);
			lightModelMatrix = glm::scale(lightModelMatrix, glm::vec3(lightRadius));

			// calculate MVP
			glm::mat4 MVP = camera->GetP() * camera->GetV() * lightModelMatrix;

			// upload data to GPU
			glUniformMatrix4fv(lightVolumeShader["MVP"], 1, GL_FALSE, glm::value_ptr(MVP));
			glUniform3fv(lightVolumeShader["lightPos"], 1, glm::value_ptr(glm::vec3(camera->GetV() * glm::vec4(light->transform.position, 1))));
			glUniform3fv(lightVolumeShader["lightColor"], 1, glm::value_ptr(light->color * light->intensity));

			// render the light volume
			glDrawElements(GL_TRIANGLES, ballModel->meshes[0]->numTriangles * 3, GL_UNSIGNED_INT, 0);
		}
		
		graphics::ToggleBlending(false);
		graphics::ToggleCulling(true);
		graphics::ToggleDepthBufferWriting(false);
		graphics::ToggleDepthTesting(true);

		skybox->Render(*camera);

		PG::Window::EndFrame();
	}

	PG::EngineQuit();

	return 0;
}
