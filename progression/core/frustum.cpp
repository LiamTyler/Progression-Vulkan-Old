#include "core/frustum.h"
#include <math.h>

namespace Progression {

    Frustum::Frustum(const Camera& camera, int row, int col, int tile_size) {
        UpdateFrustum(camera);
    }

    bool Frustum::boxInFrustum(const BoundingBox& aabb) const {
        for (int i = 0; i < 6; ++i) {
            if (!SameSide(aabb.GetP(glm::vec3(planes[i])), planes[i]))
                return false;
        }
        return true;
    }

    void Frustum::UpdateFrustum(const Camera& camera, int row, int col, int tile_size) {
        glm::vec3 ntl, ntr, nbl, nbr, ftl, ftr, fbr, fbl;
        float nearHeight, nearWidth, farHeight, farWidth;

        float angle = 0.5 * camera.GetFOV();
        float nearDist = camera.GetNearPlane();
        float farDist = camera.GetFarPlane();

        nearHeight = nearDist * tanf(angle);
        farHeight = farDist * tanf(angle);
        nearWidth = camera.GetAspectRatio() * nearHeight;
        farWidth = camera.GetAspectRatio() * farHeight;

        glm::vec3 nc = camera.transform.position + nearDist * camera.GetForwardDir();
        glm::vec3 fc = camera.transform.position + farDist * camera.GetForwardDir();
        
        glm::vec3 Y = camera.GetUpDir();
        glm::vec3 X = camera.GetRightDir();
        ntl = nc + Y * nearHeight - X * nearWidth;
        ntr = nc + Y * nearHeight + X * nearWidth;
        nbl = nc - Y * nearHeight - X * nearWidth;
        nbr = nc - Y * nearHeight + X * nearWidth;
        ftl = fc + Y * farHeight - X * farWidth;
        ftr = fc + Y * farHeight + X * farWidth;
        fbl = fc - Y * farHeight - X * farWidth;
        fbr = fc - Y * farHeight + X * farWidth;

        SetPlane(0, ntl, ntr, nbr); // near
        SetPlane(1, ftr, ftl, fbl); // far
        SetPlane(2, ntl, nbl, fbl); // left
        SetPlane(3, nbr, ntr, fbr); // right
        SetPlane(4, ntr, ntl, ftl); // top
        SetPlane(5, nbl, nbr, fbr); // bottom
    }

    bool Frustum::SameSide(const glm::vec3& point, const glm::vec4& plane) const {
        return (point.x * plane.x + point.y * plane.y + point.z * plane.z + plane.w) >= 0;
    }

    void Frustum::SetPlane(int i, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
        glm::vec3 p12 = p2 - p1;
        glm::vec3 p13 = p3 - p1;

        glm::vec3 n = glm::normalize(glm::cross(p12, p13));
        float d = glm::dot(n, -p1);
        planes[i] = glm::vec4(n, d);
    }

} // namespace Progression
