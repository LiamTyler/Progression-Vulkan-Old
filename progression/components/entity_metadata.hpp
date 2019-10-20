#pragma once

#include "core/ecs.hpp"

namespace Progression
{

struct NameComponent
{
    std::string name;
};

struct EntityMetaData
{
    entt::entity parent = entt::null;
    bool isStatic       = false;
};

} // namespace Progression
