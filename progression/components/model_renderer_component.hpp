#pragma once

#include <iostream>
#include <memory>
#include <vector>

namespace Progression
{

class Model;
class Material;

struct ModelRenderComponent
{
    std::shared_ptr< Model > model;
    std::vector< std::shared_ptr< Material > > materials;
};

void ParseModelRendererComponentFromFile( std::istream& in, ModelRenderComponent& comp );

} // namespace Progression