#pragma once

#include "core/camera.hpp"
#include <vector>

namespace Progression {

    class Light;

    class Scene {
    public:
        Scene();
        ~Scene();

        /** \brief Parses a scene file and returns a Scene object as specified in the file. */
        static Scene* load(const std::string& filename);

        void addLight(Light* light);
        void removeLight(Light* light);
        void sortLights();
        const std::vector<Light*>& getLights() const { return lights_; }
        unsigned int getNumPointLights() const { return numPointLights_; }
        unsigned int getNumSpotLights() const { return numSpotLights_; }
        unsigned int getNumDirectionalLights() const { return numDirectionalLights_; }

        Camera camera;
        glm::vec3 backgroundColor;
        glm::vec3 ambientColor;
        // std::shared_ptr<Skybox> skybox; ///< Pointer to the scene's current skybox, or nullptr

    protected:
        // TODO: Store in contiguous block. Just doing this for now to have stable pointers after
        //       adding or deleting a light
        std::vector<Light*> lights_;
        unsigned int numPointLights_       = 0; ///< Number of point lights currently in the list
        unsigned int numSpotLights_        = 0; ///< Number of spot lights currently in the list
        unsigned int numDirectionalLights_ = 0; ///< Number of directional lights currently in the list
    };

} // namespace Progression
