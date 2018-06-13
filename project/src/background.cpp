#include "include/background.h"

Background::Background() : Background(glm::vec4(1, 1, 1, 1)) {}

Background::Background(glm::vec4 c) {
    color_ = c;
    skybox_ = nullptr;
}

Background::Background(glm::vec4 c, std::vector<std::string> faces) {
    color_ = c;
    skybox_ = new Skybox("shaders/skybox.vert", "shaders/skybox.frag");
    skybox_->Load(faces);
}

Background::Background(glm::vec4 c, Skybox* sb) {
    color_ = c;
    skybox_ = sb;
}

void Background::ClearAndRender(const Camera& camera) {
    glClearColor(color_.r, color_.g, color_.b, color_.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (skybox_)
        skybox_->Render(camera);
}
