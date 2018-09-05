#include "core/scene.h"

namespace Progression {

    Scene::Scene(unsigned int maxObjects) :
        maxGameObjects_(maxObjects)
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

    // TODO: Implement view frustrum culling
    void Scene::GenerateVisibilityList() {

    }

} // namespace Progression