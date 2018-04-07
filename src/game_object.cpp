#include "include/game_object.h"

GameObject::GameObject() : GameObject(Transform(), nullptr, nullptr)
{
}

GameObject::GameObject(const Transform& t, Mesh* m, Material* mat) {
    transform = t;
	mesh = m;
	material = mat;
}

GameObject::~GameObject() {
    for (auto& c : component_list_) {
        c.second->Stop();
        delete c.second;
    }
}

void GameObject::Update(float dt) {
    for (auto& c : component_list_)
        c.second->Update(dt);
}
