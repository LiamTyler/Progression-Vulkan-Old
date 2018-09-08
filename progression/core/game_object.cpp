#include "core/game_object.h"

namespace Progression {

	GameObject::GameObject(const Transform& t) :
        transform(t)
    {
	}

	GameObject::~GameObject() {
		for (auto& c : component_list_) {
			c.second->Stop();
			delete c.second;
		}
	}

	void GameObject::Update() {
		for (auto& c : component_list_)
			c.second->Update();
	}

} // namespace Progression