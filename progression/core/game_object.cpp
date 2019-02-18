#include "core/game_object.hpp"

namespace Progression {

	GameObject::GameObject(const Transform& t, const std::string& n) :
        transform(t),
        name(n)
    {
	}

	GameObject::~GameObject() {
		for (auto& c : componentList_) {
			c.second->Stop();
			delete c.second;
		}
	}

    /*
    GameObject::GameObject(GameObject&& o) {
        *this = std::move(o);
    }

    GameObject& GameObject::operator=(GameObject&& o) {
        transform      = std::move(o.transform);
        boundingBox    = std::move(o.boundingBox);
        name           = std::move(o.name);
        componentList_ = std::move(o.componentList_);

        return *this;
    }
    */

	void GameObject::Update() {
		for (auto& c : componentList_)
			c.second->Update();
	}

} // namespace Progression
