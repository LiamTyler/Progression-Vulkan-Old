#pragma once

#include "core/game_object.hpp"
#include "graphics/lights.hpp"
#include "core/camera.hpp"
#include "graphics/skybox.hpp"
#include "graphics/material.hpp"
#include "graphics/shader.hpp"
#include "graphics/model.hpp"
#include <mutex>

namespace Progression {

    class Scene {
    public:
        Scene();
        ~Scene();

        /** \brief Parses a .pgscn file and returns a Scene object as specified in the file. */
        static Scene* load(const std::string& filename);

        /** \brief Updates all of the gameobjects, lights, the camera, and their components. */
        void update();

        /** \brief Add's the given object to the scene. */
        void addGameObject(GameObject* o);
        // /** \brief Updates all of the gameobjects, lights, the camera, and their components. */
        // void removeGameObject(GameObject* o);

        /** \brief Returns the gameobject with the given name if found, nullptr is not.
         *         Search is O(n). */
        GameObject* getGameObject(const std::string& name) const;

        // /** \brief Add's the given light to the scene. This move constructs a new light 
        //  *         given the one passed in, so the argument will be invalid upon returning. */
        // bool addLight(Light&& light);
        // /** \brief Updates all of the gameobjects, lights, the camera, and their components. */
        // void removeLight(Light* light);
        
        /** \brief Sorts the list of lights to be all point, then spot, then directional lights.*/
        void sortLights();

        Camera* getCamera() { return &camera_; }
        const std::vector<Light*>& getLights() const { return lights_; }
        const std::vector<GameObject*>& getGameObjects() const { return gameObjects_; }

        unsigned int getNumPointLights() const { return numPointLights_; }
        unsigned int getNumSpotLights() const { return numSpotLights_; }
        unsigned int getNumDirectionalLights() const { return numDirectionalLights_; }


        glm::vec4 backgroundColor; ///< The background color of the scene
        glm::vec3 ambientColor; ///< The ambient light color used in lighting calculations
        std::shared_ptr<Skybox> skybox; ///< Pointer to the scene's current skybox, or nullptr

    protected:
        std::vector<GameObject*> gameObjects_; ///< A list of all the gameobjects in the scene
        std::vector<Light*> lights_; ///< A list of all the lights in the scene
        Camera camera_; ///< The scene's camera

        unsigned int numPointLights_       = 0; ///< Number of point lights currently in the list
        unsigned int numSpotLights_        = 0; ///< Number of spot lights currently in the list
        unsigned int numDirectionalLights_ = 0; ///< Number of directional lights currently in the list


        // scene file parser helpers
        static void parseCamera(Scene* scene, std::ifstream& in);
        static void parseLight(Scene* scene, std::ifstream& in);
        static void parseGameObject(Scene* scene, std::ifstream& in);
    };

} // namespace Progression
