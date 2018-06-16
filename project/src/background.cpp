#include "include/background.h"


Background::Background(const glm::vec4& c, Skybox* sb) :
    color_(c),
    skybox_(sb)
{
}

void Background::ClearAndRender(const Camera& camera) {
    glClearColor(color_.r, color_.g, color_.b, color_.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (skybox_)
        skybox_->Render(camera);
}
