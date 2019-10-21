#pragma once

#include "resource/script.hpp"
#include "sol/sol.hpp"
#include <array>

namespace Progression
{

struct ScriptData
{
    std::shared_ptr< Script > script;
    sol::environment env;
};

class ScriptComponent
{
public:
    constexpr static int MAX_SCRIPTS_PER_COMPONENT = 5;

    ScriptComponent() = default;

    bool AddScript( const std::shared_ptr< Script >& script );

    void RemoveScript( const std::shared_ptr< Script >& script );

    int numScripts = 0;
    std::array< ScriptData, MAX_SCRIPTS_PER_COMPONENT > scripts;
};

} // namespace Progression