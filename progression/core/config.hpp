#pragma once

#include "cpptoml.h"

namespace Progression
{
namespace config
{

    class Config
    {
    public:
        Config( const std::string& fname );
        Config( std::shared_ptr< cpptoml::table >& handle );
        ~Config() = default;

        explicit operator bool() const;

        std::shared_ptr< cpptoml::table > operator->() const
        {
            return handle_;
        }

    private:
        std::shared_ptr< cpptoml::table > handle_;
    };

    Config parseFile( const std::string& path );

} // namespace config
} // namespace Progression