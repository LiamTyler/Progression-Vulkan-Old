#include "components/script_component.hpp"
#include "core/assert.hpp"
#include "core/lua.hpp"
#include "sol/sol.hpp"
#include <array>

namespace Progression
{

bool ScriptComponent::AddScript( const std::shared_ptr< Script >& script )
{
    PG_ASSERT( script );
    if ( numScripts < MAX_SCRIPTS_PER_COMPONENT )
    {
        scripts[numScripts].script = script;
        scripts[numScripts].env    = sol::environment( g_LuaState, sol::create, g_LuaState.globals() );
        g_LuaState.script( script->scriptText, scripts[numScripts].env );
        ++numScripts;
        return true;
    }
    return false;
}

void ScriptComponent::RemoveScript( const std::shared_ptr< Script >& script )
{
    PG_ASSERT( script );
    for ( int i = 0; i < numScripts; ++i )
    {
        if ( scripts[i].script == script )
        {
            scripts[i] = {}; // does this actually delete the lua environment?
            scripts[i] = scripts[numScripts - 1];
            return;
        }
    }
    PG_ASSERT( false, "Script '" + script->name + "' is not in this component" );
}

} // namespace Progression