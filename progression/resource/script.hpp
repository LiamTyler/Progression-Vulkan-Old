#pragma once

#include "resource/resource.hpp"

namespace Progression
{

class ScriptCreateInfo : public ResourceCreateInfo
{
public:
    std::string filename;
};

class Script : public Resource
{
public:
    Script() = default;

    bool Load( ResourceCreateInfo* createInfo = nullptr ) override;
    void Move( std::shared_ptr< Resource > dst ) override;
    bool Serialize( std::ofstream& outFile ) const override;
    bool Deserialize( char*& buffer ) override;

    std::string scriptText;
};

} // namespace Progression