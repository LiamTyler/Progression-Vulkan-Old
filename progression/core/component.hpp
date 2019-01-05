#pragma once

namespace Progression {

	class GameObject;

	class Component {
	public:
		Component() : Component(nullptr) {}
		Component(GameObject* obj) : gameObject(obj) {}
        virtual ~Component() = default;
		virtual void Start() = 0;
		virtual void Update() = 0;
		virtual void Stop() = 0;

		GameObject* gameObject;
	};

} // namespace Progression
