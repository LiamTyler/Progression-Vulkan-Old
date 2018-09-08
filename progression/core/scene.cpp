#include "core/scene.h"

namespace Progression {

    Scene::Scene(unsigned int maxObjects, unsigned int maxLights) :
        maxGameObjects_(maxObjects),
        maxLights_(maxLights),
        backgroundColor_(glm::vec4(.3, .3, .3, 1))
    {
    }

    Scene::~Scene() {
        for (const auto& o : gameObjects_)
            delete o;
    }

    // TODO: implement the scene loading from a file
    bool Scene::Load(const std::string& filename) {
        return false;
    }

    // TODO: implement the scene saving to a file
    bool Scene::Save(const std::string& filename) {
        return false;
    }


    void Scene::AddGameObject(GameObject* o) {
        gameObjects_.push_back(o);
    }

    void Scene::RemoveGameObject(GameObject* o) {
        const auto& iter = std::find(gameObjects_.begin(), gameObjects_.end(), o);
        if (iter != gameObjects_.end())
            gameObjects_.erase(iter);
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

} // namespace Progression