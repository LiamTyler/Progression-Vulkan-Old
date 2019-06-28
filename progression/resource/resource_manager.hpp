#pragma once

#include "resource/resource.hpp"
#include "resource/material.hpp"
#include "resource/shader.hpp"
#include "resource/texture2D.hpp"
#include <unordered_map>
#include <vector>
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
        TOTAL_RESOURCE_TYPES
    };
    
namespace ResourceManager {

    extern std::vector<std::unordered_map<std::string, Resource*>> f_resources;

    void init();
    void update();
    void waitUntilLoadComplete(const std::string& fname = "");
    void shutdown();

    template <typename T>
    T* get(const std::string& name) {
        auto& group = f_resources[getResourceTypeID<T>()];
        auto it = group.find(name);
        return it == group.end() ? nullptr : (T*) it->second;
    }

    // std::vector<Material*> loadMaterials(const std::string& fname);
    Texture2D*             loadTexture2D(const std::string& name, const TextureMetaData& metaData);
    // Model*                 loadModel(const std::string& name, const std::string& fname,
    //                                  bool optimize = true, bool freeCPUCopy = true);
    Shader*                loadShader(const std::string& name, const ShaderMetaData& metaData);

    // for all the add functions, the resource will be moved into the manager, meaning that
    // any other reference to it will be invalidated. Get the newly managed copy with the get
    // functions
    // template <typename T>
    // void add(const std::string& name, T* resource) {}

    void loadResourceFileAsync(const std::string& fname);

} } // namespace Progression::Resource
