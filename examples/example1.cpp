#include "progression.h"
#include <iomanip>

using namespace Progression;

class RotatingSpheres : public Component {
public:
	RotatingSpheres(GameObject* obj, Scene* scene, int num = 8, float radius=2, float speed=0.3) :
		Component(obj),
		rotSpeed(speed)
	{
		for (int i = 0; i < num; ++i) {
			GameObject* sphere = new GameObject;
			sphere->AddComponent<ModelRenderer>(new ModelRenderer(sphere, ResourceManager::GetModel("blueSphere").get()));
			glm::vec3 relPos(0, 1, 0);
			float angle = 2 * M_PI * i / num;
			relPos.x = radius * cos(angle);
			relPos.z = radius * sin(angle);
			sphere->transform.position = obj->transform.position + relPos;
			sphere->transform.scale = glm::vec3(.25);

			spheres_.push_back(sphere);
			pointLights_.push_back(new Light(Light::Type::POINT, spheres_[i]->transform, glm::vec3(0, 0, 1), 5));

			scene->AddGameObject(sphere);
			scene->AddLight(pointLights_[i]);
		}
	}

	~RotatingSpheres() = default;
	void Start() {}
	void Stop() {}

	void Update() {
	}

	std::vector<GameObject*> spheres_;
	std::vector<Light*> pointLights_;
	float rotSpeed;

};

int main(int argc, char* argv[]) {
	srand(time(NULL));

	auto conf = PG::config::Config(PG_ROOT_DIR "configs/default.toml");
	if (!conf) {
		std::cout << "could not parse config file" << std::endl;
		exit(0);
	}

	PG::EngineInitialize(conf);

	auto scene = Scene::Load(PG_ROOT_DIR "resources/scenes/scene1.pgscn");
	auto camera = scene->GetCamera();
	camera->AddComponent<UserCameraComponent>(new UserCameraComponent(camera, 3));

	auto robot = scene->GetGameObject("robot");
	robot->AddComponent<RotatingSpheres>(new RotatingSpheres(robot, scene));


	auto skybox = scene->getSkybox();

	Window::SetRelativeMouse(true);
	PG::Input::PollEvents();

	graphics::BindFrameBuffer(0);
	// Game loop
    while (!PG::EngineShutdown) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        if (PG::Input::GetKeyDown(PG::PG_K_ESC))
            PG::EngineShutdown = true;

        scene->Update();
		RenderSystem::Render(scene);
		skybox->Render(*camera);

        PG::Window::EndFrame();
    }

    PG::EngineQuit();

    return 0;
}