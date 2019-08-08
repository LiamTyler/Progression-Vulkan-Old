#pragma once

#include "utils/logger.hpp"
#include <functional>
#include <memory>
#include <string>
#include <sys/stat.h>

namespace Progression
{

struct ResourceCreateInfo
{
    std::string name;
};

class Resource
{
public:
    Resource() : name( "" )
    {
    }
    Resource( const std::string& n ) : name( n )
    {
    }
    virtual ~Resource() = default;

    virtual bool Load( ResourceCreateInfo* createInfo = nullptr ) = 0;
    virtual void Move( std::shared_ptr< Resource > dst )          = 0;
    virtual bool Serialize( std::ofstream& outFile ) const        = 0;
    virtual bool Deserialize( std::ifstream& in )                 = 0;

    std::string name;
};

} // namespace Progression
