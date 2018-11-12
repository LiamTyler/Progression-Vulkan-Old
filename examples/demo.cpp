#include "progression.h"
#include <iomanip>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Progression;
using namespace std;

#define NUM_SPHERES 5
GameObject* SPHERES[NUM_SPHERES];

float* genKernel(int size, float sigma) {
	float* kernel = new float[size];
	int halfSize = size / 2.0f;
	float sum = 0;
	for (int i = 0; i < size; i++) {
		kernel[i] = 1.0f / (sqrtf(2 * M_PI) * sigma)  * exp(-0.5 * pow((i - halfSize) / sigma, 2.0));
		sum += kernel[i];
	}
	for (int i = 0; i < size; i++) {
		kernel[i] /= sum;
		std::cout << std::setprecision(4) << kernel[i] << " ";
	}
	std::cout << std::endl << std::endl;

	return kernel;
}

glm::vec2 dE(const glm::vec2& pa, const glm::vec2& pb, const glm::vec2& va,
	const glm::vec2& vb, float ra, float rb)
{
	float maxt = 999;
	auto pDiff = pb - pa;
	auto vDiff = va - vb;
	// cout << "pDiff= " << pDiff << " vDiff = " << vDiff << endl;
	auto radius = ra + rb;
	auto dist = glm::length(pDiff);
	if (radius > dist)
		radius = .99 * dist;

	float a = glm::dot(vDiff, vDiff);
	float b = glm::dot(vDiff, pDiff);
	float c = glm::dot(pDiff, pDiff) - radius * radius;

	float disc = b * b - a * c;
	// cout << "disc = " << disc << ", a = " << a << ", b = " << b << ", c = " << c << endl;
	if (disc < 0 || abs(a) < 0.001)
		return glm::vec2(0);

	disc = sqrt(disc);
	float t = (b - disc) / a;
	if (t < 0 || t > maxt)
		return glm::vec2(0);

	float k = 10.5;
	float m = 2.0;
	float t0 = 3;
	return k * exp(-t / t0)*(vDiff - (vDiff*b - pDiff * a) / disc) / (a*pow(t, m))*(m / t + 1.0f / t0);
}

class RotatingSpheres : public Component {
public:
	RotatingSpheres(GameObject* obj, Scene* s, int num = 8, float r=4, float speed=0.3) :
		Component(obj),
		rotSpeed(speed),
		scene(s),
		radius(r)
	{
	}

	void AddSphere(Model* model) {
		GameObject* sphere = new GameObject;
		cout << "about to add MR component" << endl;
		cout << "model = " << model << endl;
		cout << "model mats = " << model->materials.size() << endl;
		sphere->AddComponent<ModelRenderer>(new ModelRenderer(sphere, model));
		cout << "finished adding MR component" << endl;
		//spheres.push_back(sphere);
		//pointLights.push_back(new Light(Light::Type::POINT, glm::vec3(0), model->materials[0]->emissive, 5));
		scene->AddGameObject(sphere);
		//scene->AddLight(pointLights[pointLights.size() - 1]);

		/*for (int i = 0; i < spheres.size(); ++i) {
			glm::vec3 relPos(0, gameObject->transform.position.y, 0);
			float angle = 2 * M_PI * i / spheres.size();
			relPos.x = radius * cos(angle);
			relPos.z = radius * sin(angle);
			sphere->transform.position = gameObject->transform.position + relPos;
			sphere->transform.scale = glm::vec3(.25);
		}*/
	}

	~RotatingSpheres() = default;
	void Start() {}
	void Stop() {}

	void Update() {
	}

	std::vector<GameObject*> spheres;
	std::vector<Light*> pointLights;
	Scene* scene;
	float rotSpeed;
	float radius;
};

class TTCcomponent : public Component {
public:
	TTCcomponent(GameObject* g, Scene* sc) :
		Component(g),
		scene(sc),
		radius(1.75),
		force(0),
		velocity(0),
		goalVelocity(0),
		goal(0)
	{
		int sphere = rand() % NUM_SPHERES;
		goalSphere = SPHERES[sphere];
		goal = glm::vec2(goalSphere->transform.position.x, goalSphere->transform.position.z);
		cout << "goal = " << goal << endl;
		memset(spheresReached, 0, sizeof(spheresReached));
	}

	~TTCcomponent() = default;

	void Start() {}
	void Stop() {}

	void Update() {
		glm::vec2 pos = glm::vec2(gameObject->transform.position.x, gameObject->transform.position.z);
		// cout << "pos = " << pos << endl;
		goalVelocity = 6.0f * glm::normalize(goal - pos);

		scene->GetNeighbors(gameObject, 10, neighbors);
		force = 2.0f * (goalVelocity - velocity);
		force += glm::vec2(2.0f * (rand() / (float)RAND_MAX) - 1.0f, 2.0f * (rand() / (float)RAND_MAX) - 1.0f);

		for (auto& neighbor : neighbors) {
			auto ttc = neighbor->GetComponent<TTCcomponent>();
			if (ttc) {
				glm::vec2 nPos = glm::vec2(neighbor->transform.position.x, neighbor->transform.position.z);
				// cout << "nPos = " << nPos << endl;
				auto diff = pos - nPos;
				float dist = glm::length(diff);
				float r = radius;
				if (dist < 2 * r)
					r = dist / 2.001;

				auto dEdx = dE(pos, nPos, velocity, ttc->velocity, radius, ttc->radius);
				// std::cout << dEdx << endl;
				auto FAvoid = -dEdx;
				float mag = glm::length(FAvoid);
				float maxF = 25;
				if (mag > maxF)
					FAvoid = maxF * FAvoid / mag;

				force += FAvoid;
			}
		}
	}

	void PostUpdate() {
		float dt = Time::deltaTime();
		velocity += force * dt;
		auto dP = velocity * dt;
		gameObject->transform.position += glm::vec3(dP.x, 0, dP.y);
		auto xzPos = glm::vec2(gameObject->transform.position.x, gameObject->transform.position.z);
		if (glm::length(goal - xzPos) < 2) {
			velocity = glm::vec2(0);
			cout << "call to AddSphere" << endl;
			gameObject->GetComponent<RotatingSpheres>()->AddSphere(goalSphere->GetComponent<ModelRenderer>()->model);
			for (int i = 0; i < NUM_SPHERES; ++i)
				if (goalSphere == SPHERES[i])
					spheresReached[i] = true;
			

			int reached = 0;
			for (int i = 0; i < NUM_SPHERES; ++i)
				reached += spheresReached[i];
			cout << "reached = " << reached << endl;
			int sphere = rand() % (NUM_SPHERES - reached);
			cout << "sphere = " << sphere << endl;
			reached = 0;
			for (int i = 0; i < NUM_SPHERES; ++i) {
				if (!spheresReached[i] && reached == sphere) {
					goalSphere = SPHERES[i];
					goal = glm::vec2(goalSphere->transform.position.x, goalSphere->transform.position.z);
				} else {
					reached++;
				}
			}
		}
	}

	Scene* scene;
	float radius;
	glm::vec2 force;
	glm::vec2 velocity;
	glm::vec2 goalVelocity;
	glm::vec2 goal;
	GameObject* goalSphere;
	std::vector<GameObject*> neighbors;
	bool spheresReached[NUM_SPHERES];
};

int main(int argc, char* argv[]) {
	srand(time(NULL));

	auto conf = PG::config::Config(PG_ROOT_DIR "configs/default.toml");
	if (!conf) {
		std::cout << "could not parse config file" << std::endl;
		exit(0);
	}

	PG::EngineInitialize(conf);

	std::cout << "\nbloom combine shader: " << std::endl;
	Shader bloomCombineShader = Shader(PG_ROOT_DIR "resources/shaders/bloomCombine.vert", PG_ROOT_DIR "resources/shaders/bloomCombine.frag");
	std::cout << "\nblur shader: " << std::endl;
	Shader blurShader = Shader(PG_ROOT_DIR "resources/shaders/blur.vert", PG_ROOT_DIR "resources/shaders/blur.frag");
	blurShader.AddUniform("kernel");
	std::cout << "\ncopy shader: " << std::endl;
	Shader copyShader = Shader(PG_ROOT_DIR "resources/shaders/copy.vert", PG_ROOT_DIR "resources/shaders/copy.frag");

	auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/demo.pgscn");
	const std::string sphereNames[NUM_SPHERES] = {
		"blueSphere",
		"greenSphere",
		"redSphere",
		"yellowSphere",
		"orangeSphere",
	};
	for (int i = 0; i < NUM_SPHERES; ++i) {
		SPHERES[i] = scene->GetGameObject(sphereNames[i]);
		if (SPHERES[i])
			cout << "spheres[" << i << "] = " << sphereNames[i] << endl;
	}
	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));

	auto robot = scene->GetGameObject("robot");
	robot->AddComponent<RotatingSpheres>(new RotatingSpheres(robot, scene));
	robot->AddComponent<TTCcomponent>(new TTCcomponent(robot, scene));


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
		auto& gameObjects = scene->GetGameObjects();
		for (auto& obj : gameObjects) {
			auto ttc = obj->GetComponent<TTCcomponent>();
			if (ttc) {
				ttc->PostUpdate();
			}
		}

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
