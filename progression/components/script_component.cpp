#include "components/script_component.hpp"
#include "core/assert.hpp"
#include <array>
#include "utils/logger.hpp"

namespace Progression
{

bool ScriptComponent::AddScript( const std::shared_ptr< Script >& script )
{
    PG_ASSERT( script );
    if ( numScripts < MAX_SCRIPTS_PER_COMPONENT )
    {
        ScriptData& s = scripts[numScripts];
        s.script = script;
        s.env    = sol::environment( *g_LuaState, sol::create, g_LuaState->globals() );
        g_LuaState->script( script->scriptText, s.env );
        s.updateFunc.second = s.env["Update"];
        s.updateFunc.first  = s.updateFunc.second.valid();
        if ( s.updateFunc.first )
        {
            if ( numScripts != numScriptsWithUpdate )
            {
                std::swap( scripts[numScripts], scripts[numScriptsWithUpdate] );
            }
            ++numScriptsWithUpdate;
        }
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
            if ( i < numScriptsWithUpdate )
            {
                scripts[i]                        = scripts[numScriptsWithUpdate - 1];
                scripts[numScriptsWithUpdate - 1] = scripts[numScripts - 1];
                --numScriptsWithUpdate;
            }
            else
            {
                scripts[i] = scripts[numScripts - 1];
            }
            --numScripts;
            return;
        }
    }
    PG_ASSERT( false, "Script '" + script->name + "' is not in this component" );
}

sol::function ScriptComponent::GetFunction( const std::string& scriptName, const std::string& functionName ) const
{
    int scriptIndex = -1;
    for ( int i = 0; i < numScripts; ++i )
    {
        if ( scripts[i].script->name == scriptName )
        {
            scriptIndex = i;
            break;
        }
    }
    PG_ASSERT( scriptIndex != -1, "No script found on this component with name '" + scriptName + "'" );
    sol::function ret = scripts[scriptIndex].env[functionName];
    PG_ASSERT( ret.valid(), "No function '" + functionName + "' found in script '" + scriptName + "'" );
    return ret;
}

ScriptData* ScriptComponent::GetScriptData( const std::string& scriptName )
{
    for ( int i = 0; i < numScripts; ++i )
    {
        if ( scripts[i].script->name == scriptName )
        {
            return &scripts[i];
        }
    }
    return nullptr;
}

} // namespace Progression
