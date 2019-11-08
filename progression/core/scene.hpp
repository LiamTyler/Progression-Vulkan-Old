#pragma once

#include "core/camera.hpp"
#include "core/ecs.hpp"
#include "graphics/shader_c_shared/lights.h"
#include <vector>

namespace Progression
{
    class Scene
    {
    public:
        Scene() = default;
        ~Scene();

        static Scene* Load( const std::string& filename );

        void Start();
        void Update();

        Camera camera;
        glm::vec3 backgroundColor = glm::vec3( 0, 0, 0 ); // currently not used-- hardcoded in vulkan.cpp
        glm::vec3 ambientColor    = glm::vec3( 0, 0, 0 );
        DirectionalLight directionalLight;
        std::vector< PointLight > pointLights;
        std::vector< SpotLight > spotLights;
        entt::registry registry;
    };

} // namespace Progression
