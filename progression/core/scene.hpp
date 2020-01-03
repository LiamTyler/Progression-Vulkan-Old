#pragma once

#include "core/camera.hpp"
#include "core/ecs.hpp"
#include "graphics/lights.hpp"
#include <vector>

namespace Progression
{
    class Image;
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
        std::shared_ptr< Image > skybox;
        DirectionalLight directionalLight;
        std::vector< PointLight > pointLights;
        std::vector< SpotLight > spotLights;
        entt::registry registry;
    };

} // namespace Progression
