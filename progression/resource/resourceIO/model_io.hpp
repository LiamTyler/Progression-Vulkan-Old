#pragma once

#include "resource/model.hpp"
#include <string>

namespace Progression {

    bool loadModelFromObj(Model& model, const std::string& filename,
                          bool optimize = true, bool freeCpuCopy = true);

    void optimizeMesh(Mesh& mesh);

    void optimizeModel(Model& model);

    bool loadModelInfoFromResourceFile(std::string& fname, bool& optimize, bool& freeCPUCopy, std::istream& in);

} // namespace Progression
