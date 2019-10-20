#pragma once

#include "core/platform_defines.hpp"

#if USING( DEBUG_BUILD )
#define SOL_ALL_SAFETIES_ON 1
#endif // #if USING( DEBUG_BUILD )

#include "sol/sol.hpp"

namespace Progression
{

    extern sol::state g_LuaState;

    void RegisterTypesAndFunctionsToLua( sol::state& lua );

} // namespace Progresion
