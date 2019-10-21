#pragma once

#include "core/ecs.hpp"
#include "utils/json_parsing.hpp"
#include <string>

namespace Progression
{

    void ParseComponent( rapidjson::Value& value, const entt::entity e, entt::registry& registry, const std::string& typeName );

} // namespace Progression
