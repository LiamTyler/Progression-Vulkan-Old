#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "core/transform.hpp"
#include "core/component.hpp"
#include "core/bounding_box.hpp"
#include "utils/noncopyable.hpp"

namespace Progression {

	class GameObject : public NonCopyable {
	public:
		GameObject(const Transform& t = Transform(), const std::string& name = "");
		virtual ~GameObject();

        GameObject(GameObject&& o) = default;
        GameObject& operator=(GameObject&& o) = default;

		virtual void Update();

		template<typename ComponentType>
		void AddComponent(Component* c) {
			assert(componentList_.find(typeid(ComponentType)) == componentList_.end());
			c->gameObject = this;
			c->Start();
			componentList_[typeid(ComponentType)] = c;
		}

		template<typename ComponentType>
		void RemoveComponent() {
			assert(componentList_.find(typeid(ComponentType)) != componentList_.end());
			Component* c = componentList_[typeid(ComponentType)];
			c->Stop();
			delete c;
			componentList_.erase(typeid(ComponentType));
		}

		template<typename ComponentType>
		ComponentType* GetComponent() {
			//assert(componentList_.find(typeid(ComponentType)) != componentList_.end());
            if (componentList_.find(typeid(ComponentType)) == componentList_.end())
                return nullptr;
			else
                return (ComponentType*)componentList_[typeid(ComponentType)];
		}

		Transform transform;
        BoundingBox boundingBox;
        std::string name;

	protected:
		std::unordered_map<std::type_index, Component*> componentList_;
	};

} // namespace Progression

