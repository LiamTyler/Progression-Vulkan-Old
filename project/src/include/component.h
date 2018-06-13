#pragma once

#include "include/utils.h"

class GameObject;

class Component {
    public:
        Component() : Component(nullptr) {}
        Component(GameObject* obj) : gameObject(obj) {}
        virtual ~Component() {}
        virtual void Start() = 0;
        virtual void Update(float dt) = 0;
        virtual void Stop() = 0;

        GameObject* gameObject;
};
