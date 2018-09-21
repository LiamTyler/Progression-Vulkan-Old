#pragma once

#include "core/camera.h"
#include "core/bounding_box.h"

namespace Progression {

    class Frustum {
    public:
        Frustum(const Camera& camera, int row = -1, int col = -1, int tile_size = 32);

        bool boxInFrustum(const BoundingBox& aabb) const;
        void UpdateFrustum(const Camera& camera, int row = -1, int col = -1, int tile_size = 32);

        bool SameSide(const glm::vec3& point, const glm::vec4& plane) const;

        glm::vec4 planes[6];

    private:
        void SetPlane(int i, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);

    };

} // namespace Progression