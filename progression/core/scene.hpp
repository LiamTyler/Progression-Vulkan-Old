#pragma once

#include "graphics/lights.hpp"
#include "core/camera.hpp"
#include <mutex>

namespace Progression {

    class Scene {
    public:
        Scene();
        ~Scene();

        /** \brief Parses a scene file and returns a Scene object as specified in the file. */
        static Scene* load(const std::string& filename);

        // /** \brief Add's the given light to the scene. This move constructs a new light 
        //  *         given the one passed in, so the argument will be invalid upon returning. */
        // bool addLight(Light&& light);
        // /** \brief Updates all of the gameobjects, lights, the camera, and their components. */
        // void removeLight(Light* light);

        const std::vector<Light*>& getLights() const { return lights_; }

        unsigned int getNumPointLights() const { return numPointLights_; }
        unsigned int getNumSpotLights() const { return numSpotLights_; }
        unsigned int getNumDirectionalLights() const { return numDirectionalLights_; }

        Camera camera;
        glm::vec3 backgroundColor; ///< The background color of the scene
        glm::vec3 ambientColor; ///< The ambient light color used in lighting calculations
        // std::shared_ptr<Skybox> skybox; ///< Pointer to the scene's current skybox, or nullptr

    protected:
        std::vector<Light*> lights_; ///< A list of all the lights in the scene

        unsigned int numPointLights_       = 0; ///< Number of point lights currently in the list
        unsigned int numSpotLights_        = 0; ///< Number of spot lights currently in the list
        unsigned int numDirectionalLights_ = 0; ///< Number of directional lights currently in the list
    };

} // namespace Progression
