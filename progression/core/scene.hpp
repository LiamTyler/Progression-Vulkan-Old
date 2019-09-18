#pragma once

#include "core/camera.hpp"
#include <vector>
#include <memory>

namespace Progression
{
    class Light;
    class Image;

    class Scene
    {
    public:
        Scene() = default;
        ~Scene();

        static Scene* Load( const std::string& filename );

        void AddLight( Light* light );
        void RemoveLight( Light* light );
        void SortLights();
        const std::vector< Light* >& GetLights() const;
        unsigned int GetNumPointLights() const;
        unsigned int GetNumSpotLights() const;
        unsigned int GetNumDirectionalLights() const;

        Camera camera;
        std::shared_ptr< Image > skybox;
        glm::vec3 backgroundColor = glm::vec3( 0, 0, 0 );
        glm::vec3 ambientColor    = glm::vec3( 0, 0, 0 );

    protected:
        // TODO: Store in contiguous block. Just doing this for now to have stable pointers after
        //       adding or deleting a light
        std::vector< Light* > m_lights;
        unsigned int m_numPointLights       = 0;
        unsigned int m_numSpotLights        = 0;
        unsigned int m_numDirectionalLights = 0;
    };

} // namespace Progression
