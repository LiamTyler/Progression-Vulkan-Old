#include "core/game_object.h"

namespace Progression {

	GameObject::GameObject() : GameObject(Transform())
	{
	}

	GameObject::GameObject(const Transform& t) {
		transform = t;
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

} // namespace Progression