#pragma once

#include "core/game_object.h"

namespace Progression {

    class Scene {
    public:
        explicit Scene(unsigned int maxObjects);
        ~Scene();

        bool Load(const std::string& filename);
        bool Save(const std::string& filename);

        void AddGameObject(GameObject* o);
        void RemoveGameObject(GameObject* o);

        void GenerateVisibilityList();

    protected:
        unsigned int maxGameObjects_;
        std::vector<GameObject*> gameObjects_;
    };

} // namespace Progression