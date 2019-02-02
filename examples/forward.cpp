#include "progression.hpp"

using namespace Progression;

class LightBallComponent : public Component {
public:
	LightBallComponent(GameObject* obj, GameObject* _ball) :
		Component(obj),
		ball(_ball)
	{
	}

	~LightBallComponent() = default;
	void Start() {}
	void Stop() {}

	void Update() {
		gameObject->transform = ball->transform;
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
	void Start() {}
	void Stop() {}

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

	glm::vec3 velocity;
};

int main(int argc, char* argv[]) {
	srand(time(NULL));

	auto conf = PG::config::Config(PG_ROOT_DIR "configs/bouncing_ball.toml");
	if (!conf) {
		std::cout << "could not parse config file" << std::endl;
		exit(0);
	}

	PG::EngineInitialize(conf);

	std::cout << "about to laod scene" << std::endl;
	auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/bouncing_ball.pgscn");
	std::cout << "done loading scene" << std::endl;

	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera));
	camera->SetRenderingPipeline(RenderingPipeline::FORWARD);

	auto ballModel = ResourceManager::GetModel("metalBall");
	auto planeModel = ResourceManager::GetModel("metalFloor");

	int X, Z, startX, startZ;
	float DX, DZ;

	int numBalls;
	auto val = conf->get_table("app");
	numBalls = *val->get_as<int>("numBalls");
	std::cout << "num balls" << numBalls << std::endl;
    if (numBalls == 1) {
        X = 1;
        Z = 1;
        startX = 0;
        startZ = 0;
        DX = 1;
        DZ = 1;
    }
    else if (numBalls == 9) {
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
	std::cout << "done adding components" << std::endl;
	GameObject* planeObj = scene->GetGameObject("floor");
	planeObj->AddComponent<ModelRenderer>(new ModelRenderer(planeObj, planeModel.get()));

	auto skybox = scene->getSkybox();

	// Note: After changing the input mode, should poll for events again
	Window::SetRelativeMouse(true);
	PG::Input::PollEvents();

	glEnable(GL_DEPTH_TEST);

	//while (!PG::Input::GetKeyDown(PG::PG_K_P))
	//	PG::Input::PollEvents();

	// Game loop
	while (!PG::EngineShutdown) {
		PG::Window::StartFrame();
		PG::Input::PollEvents();

		if (PG::Input::GetKeyDown(PG::PG_K_ESC))
			PG::EngineShutdown = true;

		scene->Update();

		RenderSystem::Render(scene);
		// skybox->Render(*camera);

		PG::Window::EndFrame();
	}

	PG::EngineQuit();

	return 0;
}