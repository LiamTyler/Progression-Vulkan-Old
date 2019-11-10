#pragma once

#include "resource/model.hpp"

namespace Progression
{

struct SkinnedRenderer
{
    std::shared_ptr< Model > model;
    std::vector< std::shared_ptr< Material > > materials;
};

} // namespace Progression
