#pragma once

#include "resource/model.hpp"
#include "resource/material.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace Progression
{

struct ModelRenderer
{
    std::shared_ptr< Model > model;
    std::vector< std::shared_ptr< Material > > materials;
};

} // namespace Progression
