#pragma once

#include <vector>
#include "resource/mesh.hpp"
#include "utils/noncopyable.hpp"
#include "resource/resource.hpp"

namespace Progression {

    class Material;

    class ModelMetaData {
    public:
        bool update();
        TimeStampedFile file;
        bool optimize = true;
        bool freeCpuCopy = true;
    };

    class Model : public NonCopyable, public Resource {
    public:
        Model() = default;
        Model(const std::string& name, const ModelMetaData& metaData);
        Model(Model&& model);
        Model& operator=(Model&& model);

        bool load(MetaData* metaData = nullptr) override;
        ResUpdateStatus loadFromResourceFile(std::istream& in, std::function<void()>& updateFunc) override;
        void move(Resource* resource) override;
        std::shared_ptr<Resource> needsReloading() override;

        bool loadFromObj();
        void optimize();

        ModelMetaData metaData;
        std::vector<Mesh> meshes;
        std::vector<std::shared_ptr<Material>> materials;
    };

} // namespace Progression
