#include "progression.h"
#include <iomanip>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace Progression;
using namespace std;

#define NUM_SPHERES 5
GameObject* SPHERES[NUM_SPHERES];
#define AGENT_RADIUS 3
#define ROTATING_RADIUS 2.5

glm::vec2 dE(const glm::vec2& pa, const glm::vec2& pb, const glm::vec2& va,
	const glm::vec2& vb, float ra, float rb)
{
	float maxt = 999;
	auto pDiff = pb - pa;
	auto vDiff = va - vb;
	auto radius = ra + rb;
	auto dist = glm::length(pDiff);
	if (radius > dist)
		radius = .99 * dist;

	float a = glm::dot(vDiff, vDiff);
	float b = glm::dot(vDiff, pDiff);
	float c = glm::dot(pDiff, pDiff) - radius * radius;

	float disc = b * b - a * c;
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
	RotatingSpheres(GameObject* obj, Scene* s, int num = 8, float r = ROTATING_RADIUS, float speed = 1) :
		Component(obj),
		rotSpeed(speed),
		scene(s),
		radius(r),
		totAngle(0)
	{
	}

	void AddSphere(Model* model) {
		auto sphere = new GameObject;
		sphere->AddComponent<ModelRenderer>(new ModelRenderer(sphere, model));
		auto light = new Light(Light::Type::POINT, glm::vec3(0), glm::normalize(model->materials[0]->emissive), 5);

		spheres.push_back(sphere);
		pointLights.push_back(light);
		//scene->AddGameObject(sphere);
		scene->AddLight(light);
		for (int i = 0; i < spheres.size(); ++i) {
			glm::vec3 relPos(0, gameObject->transform.position.y + 1, 0);
			float angle = 2 * M_PI * i / spheres.size();
			relPos.x = radius * cos(angle);
			relPos.z = radius * sin(angle);
			spheres[i]->transform.position = gameObject->transform.position + relPos;
			pointLights[i]->transform.position = spheres[i]->transform.position;
			spheres[i]->transform.scale = glm::vec3(.25);
		}
	}

	~RotatingSpheres() = default;
	void Start() {}
	void Stop() {}

	void Update() {
		totAngle += Time::deltaTime() * rotSpeed;
		for (int i = 0; i < spheres.size(); ++i) {
			glm::vec3 relPos(0, gameObject->transform.position.y + 1, 0);
			float angle = 2 * M_PI * i / spheres.size() + totAngle;
			relPos.x = radius * cos(angle);
			relPos.z = radius * sin(angle);
			spheres[i]->transform.position = gameObject->transform.position + relPos;
			pointLights[i]->transform.position = spheres[i]->transform.position;
		}
	}

	std::vector<GameObject*> spheres;
	std::vector<Light*> pointLights;
	Scene* scene;
	float rotSpeed;
	float radius;
	float totAngle;
};


class TTCcomponent : public Component {
public:
	TTCcomponent(GameObject* g, Scene* sc) :
		Component(g),
		scene(sc),
		radius(AGENT_RADIUS),
		force(0),
		velocity(0),
		goalVelocity(0),
		goal(0),
		spheres(0)
	{
		int sphere = rand() % NUM_SPHERES;
		goalSphere = SPHERES[sphere];
		goal = glm::vec2(goalSphere->transform.position.x, goalSphere->transform.position.z);
		memset(spheresReached, 0, sizeof(spheresReached));
	}

	~TTCcomponent() = default;

	void Start() {}
	void Stop() {}

	void Update() {
		glm::vec2 pos = glm::vec2(gameObject->transform.position.x, gameObject->transform.position.z);
		goalVelocity = 6.0f * glm::normalize(goal - pos) * (static_cast<float>(NUM_SPHERES + spheres) / NUM_SPHERES);

		scene->GetNeighbors(gameObject, 10, neighbors);
		force = 2.0f * (goalVelocity - velocity);
		force += glm::vec2(2.0f * (rand() / (float)RAND_MAX) - 1.0f, 2.0f * (rand() / (float)RAND_MAX) - 1.0f);

		for (auto& neighbor : neighbors) {
			auto ttc = neighbor->GetComponent<TTCcomponent>();
			if (ttc) {
				glm::vec2 nPos = glm::vec2(neighbor->transform.position.x, neighbor->transform.position.z);
				auto diff = pos - nPos;
				float dist = glm::length(diff);
				float r = radius;
				if (dist < 2 * r)
					r = dist / 2.001;

				auto dEdx = dE(pos, nPos, velocity, ttc->velocity, radius, ttc->radius);
				auto FAvoid = -dEdx;
				float mag = glm::length(FAvoid);
				float maxF = 25;
				if (mag > maxF)
					FAvoid = maxF * FAvoid / mag;

				force += FAvoid;
			}
		}
		//cout << force << endl;
	}

	void PostUpdate() {
		float dt = .005;
		// float dt = Time::deltaTime();
		velocity += force * dt;
		auto dP = velocity * dt;
		gameObject->transform.position += glm::vec3(dP.x, 0, dP.y);
		gameObject->transform.rotation.y += 2*dt * ((-atan2(velocity.y, velocity.x) + M_PI / 2.0f) - gameObject->transform.rotation.y);
		/*if (glm::length(gameObject->transform.position) > 200) {
			cout << gameObject->transform.position << endl;
		}*/

		auto xzPos = glm::vec2(gameObject->transform.position.x, gameObject->transform.position.z);
		if (glm::length(goal - xzPos) < 6) {
			gameObject->GetComponent<RotatingSpheres>()->AddSphere(goalSphere->GetComponent<ModelRenderer>()->model);
			spheres++;
			for (int i = 0; i < NUM_SPHERES; ++i)
				if (goalSphere == SPHERES[i])
					spheresReached[i] = true;

			int i = rand() % NUM_SPHERES;
			while (spheresReached[i])
				i = rand() % NUM_SPHERES;
			goalSphere = SPHERES[i];
			goal = glm::vec2(goalSphere->transform.position.x, goalSphere->transform.position.z);
			velocity = 6.0f * glm::normalize(goal - xzPos);


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
	int spheres;
};


int main(int argc, char* argv[]) {
	srand(time(NULL));

	auto conf = PG::config::Config(PG_ROOT_DIR "configs/default.toml");
	PG::EngineInitialize(conf);

	auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/demo.pgscn");
	const std::string sphereNames[NUM_SPHERES] = { "blueSphere", "greenSphere", "redSphere", "yellowSphere", "orangeSphere" };
	for (int i = 0; i < NUM_SPHERES; ++i)
		SPHERES[i] = scene->GetGameObject(sphereNames[i]);

	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));

	auto robotModel = ResourceManager::GetModel("robot");
	cout << "robot meshes: " << robotModel->meshes.size() << endl;
	for (auto& mesh : robotModel->meshes)
		cout << "triangles: " << mesh->numTriangles << endl;
	int SIZE = 10;
	for (int r = 0; r < SIZE; ++r) {
		for (int c = 0; c < SIZE; ++c) {
			Transform t(3.0f * glm::vec3(-SIZE + 4 * c, .1, SIZE - 4 * r), glm::vec3(0), glm::vec3(2));
			auto g = new GameObject(t);
			g->AddComponent<ModelRenderer>(new ModelRenderer(g, robotModel.get()));
			g->AddComponent<RotatingSpheres>(new RotatingSpheres(g, scene));
			g->AddComponent<TTCcomponent>(new TTCcomponent(g, scene));
			scene->AddGameObject(g);
		}
	}

	auto skybox = scene->getSkybox();

	RenderSystem::EnableOption(RenderOptions::BLOOM);
	Window::SetRelativeMouse(true);
	PG::Input::PollEvents();
	bool pause = false;
	while (!PG::EngineShutdown) {
		PG::Window::StartFrame();
		PG::Input::PollEvents();

		if (PG::Input::GetKeyDown(PG::PG_K_ESC))
			PG::EngineShutdown = true;
		if (PG::Input::GetKeyDown(PG::PG_K_P))
			pause = !pause;

		if (!pause) {
			scene->Update();
			auto& gameObjects = scene->GetGameObjects();
			for (auto& obj : gameObjects) {
				auto ttc = obj->GetComponent<TTCcomponent>();
				if (ttc)
					ttc->PostUpdate();
			}
		} else {
			camera->Update();
		}
		RenderSystem::Render(scene);


		skybox->Render(*camera);
		PG::Window::EndFrame();
	}

	PG::EngineQuit();

	return 0;
}