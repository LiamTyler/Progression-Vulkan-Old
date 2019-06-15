#pragma once

#include "resource/model.hpp"
#include <string>

namespace Progression {

    bool loadModelFromObj(Model& model, const std::string& filename,
                          bool optimize = true, bool freeCpuCopy = true);

    void optimizeMesh(Mesh& mesh);

    void optimizeModel(Model& model);

} // namespace Progression
