#pragma once

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include "core/transform.hpp"
#include "core/component.hpp"
#include "core/bounding_box.hpp"

namespace Progression {

	class GameObject {
	public:
		GameObject(const Transform& t = Transform(), const std::string& name = "");
		~GameObject();

		virtual void Update();

		template<typename ComponentType>
		void AddComponent(Component* c) {
			assert(component_list_.find(typeid(ComponentType)) == component_list_.end());
			c->gameObject = this;
			c->Start();
			component_list_[typeid(ComponentType)] = c;
		}

		template<typename ComponentType>
		void RemoveComponent() {
			assert(component_list_.find(typeid(ComponentType)) != component_list_.end());
			Component* c = component_list_[typeid(ComponentType)];
			c->Stop();
			delete c;
			component_list_.erase(typeid(ComponentType));
		}

		template<typename ComponentType>
		ComponentType* GetComponent() {
			//assert(component_list_.find(typeid(ComponentType)) != component_list_.end());
            if (component_list_.find(typeid(ComponentType)) == component_list_.end())
                return nullptr;
			else
                return (ComponentType*)component_list_[typeid(ComponentType)];
		}

		Transform transform;
        BoundingBox boundingBox;
        std::string name;

	protected:
		std::unordered_map<std::type_index, Component*> component_list_;
	};

} // namespace Progression

