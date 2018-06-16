#pragma once

#include "include/utils.h"

class Transform {
    public:
        Transform(
			const glm::vec3& pos   = glm::vec3(0),
			const glm::vec3& rot   = glm::vec3(0),
			const glm::vec3& scale = glm::vec3(1));

        glm::mat4 GetModelMatrix();

        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 scale;
};
