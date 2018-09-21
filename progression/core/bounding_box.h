#pragma once

#include "core/common.h"
#include "core/transform.h"

namespace Progression {

    class BoundingBox {
    public:
        BoundingBox(const glm::vec3& min_ = glm::vec3(0), const glm::vec3& max_ = glm::vec3(0));
        ~BoundingBox() = default;

        glm::vec3 getCenter() const;
        void getPoints(glm::vec3* data) const;
        void setCenter(const glm::vec3& point);
        void Encompass(const BoundingBox& aabb, const Transform& transform);
        void Render() const;
        glm::mat4 GetModelMatrix() const;
        glm::vec3 GetP(const glm::vec3& planeNormal) const;
        glm::vec3 GetN(const glm::vec3& planeNormal) const;

        glm::vec3 min;
        glm::vec3 max;
        glm::vec3 extent;
    };

} // namespace Progression