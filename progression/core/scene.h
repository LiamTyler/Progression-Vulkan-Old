#pragma once

#include "core/game_object.h"
#include "types/lights.h"
#include "core/camera.h"
#include "graphics/skybox.h"

namespace Progression {

    class Scene {
    public:
        Scene(unsigned int maxObjects = 1000000, unsigned int maxLights = 400);
        ~Scene();

        bool Load(const std::string& filename);
        bool Save(const std::string& filename);

        void Update();

        void AddGameObject(GameObject* o);
        void RemoveGameObject(GameObject* o);

        bool AddLight(Light* light);
        void RemoveLight(Light* light);

        void AddCamera(Camera* camera, bool setMain = false);
        void RemoveCamera(Camera* camera);
        Camera* GetCamera(int index = 0) { return cameras_[index]; }

        Skybox* getSkybox() const { return skybox_; }
        void setSkybox(Skybox* skybox) { skybox_ = skybox; }

        void GenerateVisibilityList();

        std::vector<Light*>& GetPointLights() { return pointLights_; }
        std::vector<Light*>& GetDirectionalLights() { return directionalLights_; }
        unsigned int GetNumPointLights() const { return pointLights_.size(); }
        unsigned int GetNumDirectionalLights() const { return directionalLights_.size(); }
        const std::vector<GameObject*>& GetGameObjects() const { return gameObjects_; }
        glm::vec4 GetBackgroundColor() const { return backgroundColor_; }
        void SetBackgroundColor(const glm::vec4& color) { backgroundColor_ = color; }

    protected:
        unsigned int maxGameObjects_;
        unsigned int maxLights_;

        std::vector<GameObject*> gameObjects_;
        std::vector<Light*> directionalLights_;
        std::vector<Light*> pointLights_;
        std::vector<Camera*> cameras_;

        glm::vec4 backgroundColor_;
        Skybox* skybox_;
    };

} // namespace Progression