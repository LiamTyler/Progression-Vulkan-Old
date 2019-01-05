#include "core/game_object.hpp"

namespace Progression {

	GameObject::GameObject(const Transform& t, const std::string& n) :
        transform(t),
        name(n)
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
