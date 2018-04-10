#pragma once

#include "include/utils.h"
#include "include/transform.h"
#include "include/component.h"
#include "include/mesh.h"
#include "include/material.h"

#include <typeindex>
#include <typeinfo>

class GameObject {
    public:
        GameObject();
        GameObject(const Transform& t);
		~GameObject();

        void Update(float dt);

        template<typename ComponentType>
        void AddComponent(Component* c);

        template<typename ComponentType>
        void RemoveComponent();

        template<typename ComponentType>
        ComponentType* GetComponent();
        
        Transform transform;

    protected:
        std::unordered_map<std::type_index, Component*> component_list_;
};

template<typename ComponentType>
ComponentType* GameObject::GetComponent() {
    assert(component_list_.find(typeid(ComponentType)) != component_list_.end());
    return (ComponentType*) component_list_[typeid(ComponentType)];
}

template<typename ComponentType>
void GameObject::AddComponent(Component* c) {
    assert(component_list_.find(typeid(ComponentType)) == component_list_.end());
    c->gameObject = this;
    c->Start();
    component_list_[typeid(ComponentType)] = c;
}

template<typename ComponentType>
void GameObject::RemoveComponent() {
    assert(component_list_.find(typeid(ComponentType)) != component_list_.end());
    Component* c = component_list_[typeid(ComponentType)];
    c->Stop();
    delete c;
    component_list_.erase(typeid(ComponentType));
}
