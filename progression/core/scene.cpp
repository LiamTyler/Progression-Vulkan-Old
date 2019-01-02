#include "core/scene.hpp"
#include <fstream>
#include <sstream>
// #include <filesystem>
#include "core/resource_manager.hpp"
#include "graphics/model_render_component.hpp"

namespace Progression {

    Scene::Scene(unsigned int maxObjects, unsigned int maxLights) :
        maxGameObjects_(maxObjects),
        maxLights_(maxLights),
        backgroundColor_(glm::vec4(.3, .3, .3, 1)),
        skybox_(nullptr)
    {
    }

    Scene::~Scene() {
        for (const auto& o : gameObjects_)
            delete o;
    }

	// TODO: Fix the need for a newline at the end of a file (if this is still a problem)
    Scene* Scene::Load(const std::string& filename) {
        // std::filesystem::path path(filename);
        // if (!std::filesystem::exists(path)) {
        //     std::cout << "File does not exist: " << path << std::endl;
        //     return nullptr;
        // }

        Scene* scene = new Scene;

        std::ifstream in(filename);
        if (!in) {
            std::cout << "File does not exist: " << filename << std::endl;
            return nullptr;
        }
        std::string line;
        while (std::getline(in, line)) {
            if (line == "")
                continue;
            if (line[0] == '#')
                continue;
            if (line == "Resource-file") {
                std::getline(in, line);
                std::stringstream ss(line);
                std::string filename;
                ss >> filename;
                ss >> filename;
                ResourceManager::LoadResourceFile(filename);
            } else if (line == "Camera") {
                ParseCamera(scene, in);
            } else if (line == "Light") {
                ParseLight(scene, in);
            } else if (line == "GameObject") {
                ParseGameObject(scene, in);
            } else if (line == "Skybox") {
                std::getline(in, line);
                std::stringstream ss(line);
                std::string name;
                ss >> name;
                ss >> name;
                scene->skybox_ = ResourceManager::GetSkybox(name);
            }
        }

        in.close();

        return scene;
    }

    // TODO: implement the scene saving to a file
    bool Scene::Save(const std::string& filename) {
        return false;
    }

    void Scene::Update() {
        for (const auto& obj : gameObjects_)
            obj->Update();
        for (const auto& l : directionalLights_)
            l->Update();
        for (const auto& l : pointLights_)
            l->Update();
        for (const auto& c : cameras_)
            c->Update();
    }

    void Scene::AddGameObject(GameObject* o) {
        gameObjects_.push_back(o);
    }

    GameObject* Scene::GetGameObject(const std::string& name) const {
        for (const auto& obj : gameObjects_)
            if (obj->name == name)
                return obj;
        return nullptr;
    }

    void Scene::RemoveGameObject(GameObject* o) {
        const auto& iter = std::find(gameObjects_.begin(), gameObjects_.end(), o);
        if (iter != gameObjects_.end())
            gameObjects_.erase(iter);
    }

    void Scene::GetNeighbors(GameObject* o, float radius, std::vector<GameObject*>& neighborList) {
        neighborList.clear();
        float r2 = radius*radius;
        auto& pos = o->transform.position;
        for (auto& obj : gameObjects_) {
            auto diff = obj->transform.position - pos;
            if (obj != o && glm::dot(diff, diff) < r2)
                neighborList.push_back(obj);
        }
    }

    bool Scene::AddLight(Light* light) {
        if (directionalLights_.size() + pointLights_.size() == maxLights_)
            return false;
        if (light->type == Light::Type::DIRECTIONAL) {
            directionalLights_.push_back(light);
        } else if (light->type == Light::Type::POINT) {
            pointLights_.push_back(light);
        }
        return true;
    }

    // TODO: should this delete the light?
    void Scene::RemoveLight(Light* light) {
        if (light->type == Light::Type::DIRECTIONAL) {
            const auto& iter = std::find(directionalLights_.begin(), directionalLights_.end(), light);
            if (iter != directionalLights_.end())
                directionalLights_.erase(iter);
                
        } else if (light->type == Light::Type::POINT) {
            const auto& iter = std::find(pointLights_.begin(), pointLights_.end(), light);
            if (iter != pointLights_.end())
                pointLights_.erase(iter);
        }
    }

    void Scene::AddCamera(Camera* camera, bool setMain) {
        if (setMain) {
            cameras_.insert(cameras_.begin(), camera);
        } else {
            cameras_.push_back(camera);
        }
    }

    void Scene::RemoveCamera(Camera* camera) {
        const auto& iter = std::find(cameras_.begin(), cameras_.end(), camera);
        if (iter != cameras_.end())
            cameras_.erase(iter);
    }

    // TODO: Implement view frustrum culling
    void Scene::GenerateVisibilityList() {

    }

    void Scene::ParseCamera(Scene* scene, std::ifstream& in) {
        Camera* camera = new Camera;
        std::string line = " ";
        bool primaryCamera = false;
        while (line != "" && !in.eof()) {
            std::getline(in, line);
            std::stringstream ss(line);
            std::string first;
            ss >> first;
            if (first == "name") {
                ss >> camera->name;
            } else if (first == "position") {
                float x, y, z;
                ss >> x >> y >> z;
                camera->transform.position = glm::vec3(x, y, z);
            } else if (first == "rotation") {
                float x, y, z;
                ss >> x >> y >> z;
                camera->transform.rotation = glm::vec3(glm::radians(x), glm::radians(y), glm::radians(z));
            } else if (first == "scale") {
                float x, y, z;
                ss >> x >> y >> z;
                camera->transform.scale = glm::vec3(x, y, z);
            } else if (first == "fov") {
                float x;
                ss >> x;
                camera->SetFOV(glm::radians(x));
            } else if (first == "aspect") {
                float width, height;
                char colon;
                ss >> width >> colon >> height;
                camera->SetAspectRatio(width / height);
            } else if (first == "near-plane") {
                float x;
                ss >> x;
                camera->SetNearPlane(x);
            } else if (first == "far-plane") {
                float x;
                ss >> x;
                camera->SetFarPlane(x);
            } else if (first == "primary-camera") {
                std::string tmp;
                ss >> tmp;
                if (tmp == "true")
                    primaryCamera = true;
            } else if (first == "rendering-pipeline") {
				std::string tmp;
				ss >> tmp;
				RenderingPipeline pipeline;
				if (tmp == "forward")
					pipeline = RenderingPipeline::FORWARD;
				else if (tmp == "tiled-deferred")
					pipeline = RenderingPipeline::TILED_DEFERRED;
				camera->SetRenderingPipeline(pipeline);
			}
        }

        scene->AddCamera(camera, primaryCamera);
    }

    void Scene::ParseLight(Scene* scene, std::ifstream& in) {
        Light* light = new Light;
        std::string line = " ";
        while (line != "" && !in.eof()) {
            std::getline(in, line);
            std::stringstream ss(line);
            std::string first;
            ss >> first;
            if (first == "name") {
                ss >> light->name;
            } else if (first == "type") {
                std::string type;
                ss >> type;
                if (type == "directional")
                    light->type = Light::Type::DIRECTIONAL;
            } else if (first == "intensity") {
                ss >> light->intensity;
            } else if (first == "color") {
                float x, y, z;
                ss >> x >> y >> z;
                light->color = glm::vec3(x, y, z);
            } else if (first == "position") {
                float x, y, z;
                ss >> x >> y >> z;
                light->transform.position = glm::vec3(x, y, z);
            } else if (first == "rotation") {
                float x, y, z;
                ss >> x >> y >> z;
                light->transform.rotation = glm::vec3(glm::radians(x), glm::radians(y), glm::radians(z));
            } else if (first == "scale") {
                float x, y, z;
                ss >> x >> y >> z;
                light->transform.scale = glm::vec3(x, y, z);
            }
        }
        scene->AddLight(light);
    }

    void Scene::ParseGameObject(Scene* scene, std::ifstream& in) {
        GameObject* obj = new GameObject;
        std::string line = " ";
        while (line != "" && !in.eof()) {
            std::getline(in, line);
            std::stringstream ss(line);
            std::string first;
            ss >> first;
            if (first == "name") {
                ss >> obj->name;
            } else if (first == "position") {
                float x, y, z;
                ss >> x >> y >> z;
                obj->transform.position = glm::vec3(x, y, z);
            } else if (first == "rotation") {
                float x, y, z;
                ss >> x >> y >> z;
                obj->transform.rotation = glm::vec3(glm::radians(x), glm::radians(y), glm::radians(z));
            } else if (first == "scale") {
                float x, y, z;
                ss >> x >> y >> z;
                obj->transform.scale = glm::vec3(x, y, z);
            } else if (first == "model") {
				std::string modelName;
				ss >> modelName;
				auto model = ResourceManager::GetModel(modelName);
				obj->AddComponent<ModelRenderer>(new ModelRenderer(obj, model.get()));
            }
        }
        scene->AddGameObject(obj);
    }

} // namespace Progression
