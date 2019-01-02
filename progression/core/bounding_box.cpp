#include "core/bounding_box.hpp"

namespace Progression {

    BoundingBox::BoundingBox(const glm::vec3& min_, const glm::vec3& max_) :
        min(min_),
        max(max_),
        extent(max_ - min_)
    {
    }

    glm::vec3 BoundingBox::getCenter() const {
        return (max + min) / 2.0f;
    }

    void BoundingBox::getPoints(glm::vec3* data) const {
        data[0] = min + glm::vec3(0);
        data[1] = min + glm::vec3(extent.x, 0, 0);
        data[2] = min + glm::vec3(extent.x, 0, extent.z);
        data[3] = min + glm::vec3(0, 0, extent.z);
        data[4] = max - glm::vec3(0);
        data[5] = max - glm::vec3(extent.x, 0, 0);
        data[6] = max - glm::vec3(extent.x, 0, extent.z);
        data[7] = max - glm::vec3(0, 0, extent.z);
    }

    void BoundingBox::setCenter(const glm::vec3& point) {
        min = point - extent / 2.0f;
        max = point + extent / 2.0f;
    }

    void BoundingBox::Encompass(const BoundingBox& aabb, const Transform& transform) {
        glm::vec3 he = aabb.extent / 2.0f;
        glm::vec3 points[8] = {
            -he,
            glm::vec3(he.x, -he.y, -he.z),
            glm::vec3(-he.x, -he.y, he.z),
            glm::vec3(he.x, -he.y, he.z),
            he,
            glm::vec3(he.x, he.y, -he.z),
            glm::vec3(-he.x, he.y, he.z),
            glm::vec3(-he.x, he.y, -he.z)
        };

        glm::mat4 M = transform.GetModelMatrix();
        min = max = glm::vec3(M * glm::vec4(points[0], 1));
        for (int i = 1; i < 8; ++i) {
            glm::vec3 tmp = glm::vec3(M * glm::vec4(points[i], 1));
            if (tmp.x > max.x)
                max.x = tmp.x;
            else if (tmp.x < min.x)
                min.x = tmp.x;
            if (tmp.y > max.y)
                max.y = tmp.y;
            else if (tmp.y < min.y)
                min.y = tmp.y;
            if (tmp.z > max.z)
                max.z = tmp.z;
            else if (tmp.z < min.z)
                min.z = tmp.z;
        }
        extent = max - min;
    }

    glm::mat4 BoundingBox::GetModelMatrix() const {
        glm::mat4 model(1);
        glm::vec3 center = .5f * (min + max);
        model = glm::translate(model, center);
        model = glm::scale(model, 0.5f * extent);
        return model;
    }

    glm::vec3 BoundingBox::GetP(const glm::vec3& planeNormal) const {
        glm::vec3 P = min;
        if (planeNormal.x >= 0)
            P.x = max.x;
        if (planeNormal.y >= 0)
            P.y = max.y;
        if (planeNormal.z >= 0)
            P.z = max.z;

        return P;
    }

    glm::vec3 BoundingBox::GetN(const glm::vec3& planeNormal) const {
        glm::vec3 N = max;
        if (planeNormal.x >= 0)
            N.x = min.x;
        if (planeNormal.y >= 0)
            N.y = min.y;
        if (planeNormal.z >= 0)
            N.z = min.z;

        return N;
    }

} // namespace Progression