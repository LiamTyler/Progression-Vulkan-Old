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
    std::pair< bool, sol::function > updateFunc;
};

class ScriptComponent
{
public:
    constexpr static int MAX_SCRIPTS_PER_COMPONENT = 5;

    ScriptComponent() = default;

    bool AddScript( const std::shared_ptr< Script >& script );

    void RemoveScript( const std::shared_ptr< Script >& script );

    sol::function GetFunction( const std::string& scriptName, const std::string& functionName ) const;

    int numScripts           = 0;
    int numScriptsWithUpdate = 0;
    // std::array< ScriptData, MAX_SCRIPTS_PER_COMPONENT > scripts;
    ScriptData scripts[MAX_SCRIPTS_PER_COMPONENT];
};

} // namespace Progression
