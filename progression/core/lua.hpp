#pragma once

#include "core/platform_defines.hpp"

#if USING( DEBUG_BUILD )
#define SOL_ALL_SAFETIES_ON 1
#endif // #if USING( DEBUG_BUILD )

#include "sol/sol.hpp"

void RegisterTypesAndFunctionsToLua( sol::state& lua );