#pragma once

#include "core/config.h"

#include <typeindex>
#include <typeinfo>
#include <unordered_map>

namespace Progression {

    class Scene;
    class RenderSubSystem;

    class RenderSystem {
    public:
        RenderSystem() = delete;
        virtual ~RenderSystem() = delete;

        static void Init(const config::Config& config);
        static void Free();

        static void Render(Scene* scene);

        template<typename T>
        static T* GetSubSystem() {
            if (subSystems_.find(typeid(T)) == subSystems_.end())
                return nullptr;
            else
                return (T*) subSystems_[typeid(T)];
        }

    protected:
        static std::unordered_map<std::type_index, RenderSubSystem*> subSystems_;
    };

} // namespace Progression