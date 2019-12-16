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
        glm::vec4 backgroundColor = glm::vec4( 0, 0, 0, 1 );
        glm::vec3 ambientColor    = glm::vec3( .1f );
        DirectionalLight directionalLight;
        std::vector< PointLight > pointLights;
        std::vector< SpotLight > spotLights;
        entt::registry registry;
    };

} // namespace Progression
