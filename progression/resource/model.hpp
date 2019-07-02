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

        virtual bool load(MetaData* metaData = nullptr) override;
        virtual bool loadFromResourceFile(std::istream& in) override;
        virtual void move(Resource* resource) override;
        virtual std::shared_ptr<Resource> needsReloading() override;

        bool loadFromObj();
        void optimize();

        ModelMetaData metaData;
        std::vector<Mesh> meshes;
        std::vector<std::shared_ptr<Material>> materials;
    };

} // namespace Progression
