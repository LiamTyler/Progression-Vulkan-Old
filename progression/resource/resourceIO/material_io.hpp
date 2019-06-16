#pragma once

#include "resource/material.hpp"
#include "resource/texture2D.hpp"
#include <vector>
#include <unordered_map>

namespace Progression {

    /** \brief Parses the mtl file and fills the
     *
     * Any textures that it runs into are automatically loaded with the default parameters unless
     * it is currently found in the resource manager already. New texture's cpu copies are freed
     *
     * \return false if any of the mtl or texture files were unable to be opened. true otherwise
     */
    bool loadMtlFile(
            std::vector<std::pair<std::string, Material>>& materials,
            const std::string& fname,
            const std::string& rootTexDir = "");


    /** \brief Fill's the given material with the data found in the resource file.
     *
     * Any textures encountered while parsing the material need to already be loaded into the manager
     */
    bool loadMaterialFromResourceFile(std::istream& in);

} // namespace Progression
