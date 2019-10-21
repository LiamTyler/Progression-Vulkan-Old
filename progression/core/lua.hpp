#pragma once

#include "core/platform_defines.hpp"

#if USING( DEBUG_BUILD )
#define SOL_ALL_SAFETIES_ON 1
#endif // #if USING( DEBUG_BUILD )

#include "sol/sol.hpp"
#include "utils/logger.hpp"

#if USING( DEBUG_BUILD )
#define CHECK_SOL_FUNCTION_CALL( statement ) \
    { \
        sol::function_result res = statement; \
        if ( !res.valid() ) \
        { \
            sol::error err = res; \
		    std::string what = err.what(); \
            LOG_ERR( "Sol function failed with error: '", what, "'" ); \
        } \
    }    
#else // #if USING( DEBUG_BUILD )
    #define CHECK_SOL_FUNCTION_CALL( statement ) statement
#endif // #if USING( DEBUG_BUILD )

namespace Progression
{

    extern sol::state g_LuaState;

    void RegisterTypesAndFunctionsToLua( sol::state& lua );

} // namespace Progresion
