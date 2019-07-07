#pragma once

#include "resource/resource.hpp"
#include <unordered_map>
#include <vector>
#include <memory>
#include "utils/logger.hpp"

class ResourceTypeID {
    inline static uint32_t identifier;

    template<typename...>
    inline static const auto inner = identifier++;

public:

    template<typename... Type>
    inline static const uint32_t id = inner<std::decay_t<Type>...>;
};

template <typename Resource>
uint32_t getResourceTypeID() {
    return ResourceTypeID::id<Resource>;
}

namespace Progression {

    enum ResourceTypes {
        SHADER = 0,
        TEXTURE2D,
        MATERIAL,
        MODEL,
        TOTAL_RESOURCE_TYPES
    };
    
namespace ResourceManager {

    using ResourceMap = std::unordered_map<std::string, std::shared_ptr<Resource>>;
    using UpdateMap = std::unordered_map<std::string, std::function<void()>>;
    using ResourceDB = std::vector<ResourceMap>;
    using UpdateDB = std::vector<UpdateMap>;

    extern ResourceDB f_resources;

    void init(bool scanner = true);
    void update();
    bool resolveSoftLinks(ResourceDB& db);
    bool waitUntilLoadComplete(const std::string& fname = "");
    void shutdown();

    template <typename T>
    std::shared_ptr<T> get(const std::string& name) {
        auto& group = f_resources[getResourceTypeID<T>()];
        auto it = group.find(name);
        return it == group.end() ? nullptr : std::static_pointer_cast<T>(it->second);
    }

    template <typename T>
    std::shared_ptr<T> loadInternal(const std::string& name, MetaData* metaData, ResourceDB* db) {
        static_assert(std::is_base_of<Resource, T>::value && !std::is_same<Resource, T>::value, "Can only call load with a class that inherits from Resource");
        auto& DB = (*db)[getResourceTypeID<T>()];
        T resource;
        resource.name = name;
        if (!resource.load(metaData)) {
            LOG_ERR("Failed to load resource with name '", name, "'");
            return nullptr;
        }

        if (DB.find(name) != DB.end()) {
            *(DB[name]) = std::move(resource);
        } else {
            DB[name] = std::make_shared<T>(std::move(resource));
        }

        return std::static_pointer_cast<T>(DB[name]);
    }

    template <typename T>
    std::shared_ptr<T> load(const std::string& name, MetaData* metaData = nullptr) {
        return loadInternal<T>(name, metaData, &f_resources);
    }

    // for all the add functions, the resource will be moved into the manager, meaning that
    // any other reference to it will be invalidated. Get the newly managed copy with the get
    // functions
    // template <typename T>
    // void add(const std::string& name, T* resource) {}

    void loadResourceFileAsync(const std::string& fname);

    // meant to be called without any other resources loaded already
    bool createFastFile(const std::string& resourceFile);

    bool loadFastFile(const std::string& fastFile);

} } // namespace Progression::Resource
