#pragma once

#include "include/utils.h"
#include "include/skybox.h"

class Background {
    public:
        Background();
        Background(glm::vec4 c);
        Background(glm::vec4 c, std::vector<std::string> faces);
        Background(glm::vec4 c, Skybox* sb);

        void ClearAndRender(const Camera& camera);

        glm::vec4 GetColor() { return color_; }
        void SetColor(const glm::vec4& c) { color_ = c; }

    private:
        glm::vec4 color_;
        Skybox* skybox_;
};
