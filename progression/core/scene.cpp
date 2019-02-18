#include "core/scene.hpp"
#include <fstream>
#include <sstream>
#include "core/resource_manager.hpp"
#include "graphics/model_render_component.hpp"
#include "utils/logger.hpp"
#include "graphics/render_system.hpp"
#include "graphics/shadow_map.hpp"

namespace Progression {

    Scene::Scene(unsigned int maxObjects, unsigned int maxLights) :
        backgroundColor(glm::vec4(.3, .3, .3, 1)),
        maxGameObjects_(maxObjects),
        maxLights_(maxLights),
        skybox_(nullptr)
    {
    }

    Scene::~Scene() {
        for (const auto& o : gameObjects_)
            delete o;
        for (const auto& l : directionalLights_)
            delete l;
        for (const auto& l : pointLights_)
            delete l;
        for (const auto& l : spotLights_)
            delete l;
        for (const auto& c : cameras_)
            delete c;
    }

    Scene* Scene::Load(const std::string& filename) {
        Scene* scene = new Scene;

        std::ifstream in(filename);
        if (!in) {
            LOG_ERR("Could not open scene:", filename);
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
                std::string fname;
                ss >> fname;
                ss >> fname;
                ResourceManager::LoadResourceFile(fname);
            } else if (line == "RootResourceDir") {
                std::getline(in, line);
                std::stringstream ss(line);
                ss >> ResourceManager::rootResourceDir;
            } else if (line == "Camera") {
                parseCamera(scene, in);
            } else if (line == "Light") {
                parseLight(scene, in);
            } else if (line == "GameObject") {
                parseGameObject(scene, in);
            } else if (line == "Skybox") {
                std::getline(in, line);
                std::stringstream ss(line);
                std::string name;
                ss >> name;
                ss >> name;
                scene->skybox_ = ResourceManager::GetSkybox(name);
            } else if (line == "AmbientLight") {
                // next line should be "color _ _ _"
                std::getline(in, line);
                std::stringstream ss(line);
                std::string name;
                ss >> name;
                ss >> scene->ambientColor;
            } else if (line == "BackgroundColor") {
                // next line should be "color _ _ _"
                std::getline(in, line);
                std::stringstream ss(line);
                std::string name;
                ss >> name;
                ss >> scene->backgroundColor;
            }
        }

        in.close();

        return scene;
    }

    void Scene::Update() {
        for (const auto& obj : gameObjects_)
            obj->Update();
        for (const auto& l : lights_)
            l->Update();
        camera_.Update();
    }

    GameObject* Scene::getGameObject(const std::string& name) const {
        for (const auto& obj : gameObjects_)
            if (obj->name == name)
                return obj;
        return nullptr;
    }

    /*
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
    */

    void Scene::parseCamera(Scene* scene, std::ifstream& in) {
        Camera& camera = scene->camera_;
        std::string line = " ";
        bool primaryCamera = false;
        while (line != "" && !in.eof()) {
            std::getline(in, line);
            std::stringstream ss(line);
            std::string first;
            ss >> first;
            if (first == "name") {
                ss >> camera.name;
            } else if (first == "position") {
                float x, y, z;
                ss >> x >> y >> z;
                camera.transform.position = glm::vec3(x, y, z);
            } else if (first == "rotation") {
                float x, y, z;
                ss >> x >> y >> z;
                camera.transform.rotation = glm::vec3(glm::radians(x), glm::radians(y), glm::radians(z));
            } else if (first == "scale") {
                float x, y, z;
                ss >> x >> y >> z;
                camera.transform.scale = glm::vec3(x, y, z);
            } else if (first == "fov") {
                float x;
                ss >> x;
                camera.SetFOV(glm::radians(x));
            } else if (first == "aspect") {
                float width, height;
                char colon;
                ss >> width >> colon >> height;
                camera.SetAspectRatio(width / height);
            } else if (first == "near-plane") {
                float x;
                ss >> x;
                camera.SetNearPlane(x);
            } else if (first == "far-plane") {
                float x;
                ss >> x;
                camera.SetFarPlane(x);
            } else if (first == "exposure") {
                ss >> camera.renderOptions.exposure;
			}
        }
    }

    void Scene::parseLight(Scene* scene, std::ifstream& in) {
        Light* light = new Light;
        std::string line = " ";
        glm::ivec2 shadowResolution(-1, -1);
        bool shadows = false;
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
                if (type == "directional") {
                    light->type = Light::Type::DIRECTIONAL;
                } else if (type == "spot") {
                    light->type = Light::Type::SPOT;
                } else {
                    light->type = Light::Type::POINT;
                }
            } else if (first == "shadows") {
                std::string tmp;
                ss >> tmp;
                shadows = tmp == "true";
            } else if (first == "shadowMapResolution") {
                ss >> shadowResolution.x >> shadowResolution.y;
            } else if (first == "intensity") {
                ss >> light->intensity;
            } else if (first == "radius") {
                ss >> light->radius;
            } else if (first == "innerCutoff") {
                float degrees;
                ss >> degrees;
                light->innerCutoff = glm::radians(degrees);
            } else if (first == "outerCutoff") {
                float degrees;
                ss >> degrees;
                light->outerCutoff = glm::radians(degrees);
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

        auto shadowType = light->type == Light::Type::POINT ? ShadowMap::Type::CUBE : ShadowMap::Type::QUAD;

        if (shadows) {
            if (shadowResolution.x != -1)
                light->shadowMap = new ShadowMap(shadowType, shadowResolution.x, shadowResolution.y);
            else
                light->shadowMap = new ShadowMap(shadowType);
        }
        scene->AddLight(light);
    }

    void Scene::parseGameObject(Scene* scene, std::ifstream& in) {
        GameObject* obj = new GameObject;
        std::string line = " ";
        std::shared_ptr<Material> material;
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
                if (!model) {
                    LOG_ERR("No model found with name: ", modelName);
                    exit(EXIT_FAILURE);
                }
				obj->AddComponent<ModelRenderer>(new ModelRenderer(obj, model.get()));
            } else if (first == "material") {
                std::string materialName;
                ss >> materialName;
                material = ResourceManager::GetMaterial(materialName);
                if (!material) {
                    LOG_WARN("No match for material", materialName, "in the resource manager");
                }
            }
            // TODO: Specify material from here, which will help with built in shapes
        }

        // TODO: add back in
        // auto modelRenderer = obj->GetComponent<ModelRenderer>();
        // if (modelRenderer && material) {
        //     for (size_t i = 0; i < modelRenderer->meshRenderers.size(); ++i)
        //         modelRenderer->meshRenderers[i]->material = material.get();
        // }
        scene->AddGameObject(obj);
    }

} // namespace Progression
