#pragma once

#include "resource/material.hpp"
#include "resource/texture2D.hpp"
#include <vector>
#include <unordered_map>

namespace Progression {

    static std::unordered_map<std::string, Texture2D> defaultEmptyMap;

    /** \brief Parses the mtl file and fills the
     *
     * Any textures that it runs into are automatically loaded with the default parameters unless
     * it is currently found in the map provided
     *
     * \return false if any of the mtl or texture files were unable to be opened. true otherwise
     */
    bool loadMtlFile(
            std::vector<std::pair<std::string, Material>>& materials,
            const std::string& fname,
            const std::string& rootTexDir = "",
            std::unordered_map<std::string, Texture2D>& existingTextureList = defaultEmptyMap);

} // namespace Progression
