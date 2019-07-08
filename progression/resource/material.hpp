#pragma once

#include "core/common.hpp"
#include "resource/resource.hpp"

namespace Progression {

    class Texture2D;

    class Material : public Resource {
    public:
        Material() = default;

        bool load(MetaData* metaData = nullptr) override;
        bool readMetaData(std::istream& in) override;
        ResUpdateStatus loadFromResourceFile(std::istream& in, std::function<void()>& updateFunc) override;
        void move(Resource* resource) override;
        bool saveToFastFile(std::ofstream& outFile) const override;
        bool loadFromFastFile(std::ifstream& in) override;
        static bool loadMtlFile(
            std::vector<std::pair<std::string, Material>>& materials,
            const std::string& fname,
            const std::string& rootTexDir = "");

        glm::vec3 Ka;
        glm::vec3 Kd;
        glm::vec3 Ks;
		glm::vec3 Ke;
        float Ns;
        std::string map_Kd_name;
        Texture2D* map_Kd = nullptr;
    };

} // namespace Progression
