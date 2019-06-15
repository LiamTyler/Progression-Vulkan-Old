#pragma once

#include "resource/model.hpp"
#include <string>

namespace Progression {

    bool loadModelFromObj(Model& model, const std::string& filename, bool freeCpuCopy = true);

} // namespace Progression
