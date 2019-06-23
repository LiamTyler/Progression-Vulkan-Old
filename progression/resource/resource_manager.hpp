#pragma once

#include "resource/resource.hpp"
#include "resource/material.hpp"
#include "resource/shader.hpp"
#include "resource/texture2D.hpp"
#include <unordered_map>
#include <vector>
#include "utils/logger.hpp"

template<typename...>
class ResourceTypeID {
    inline static uint32_t identifier;

    template<typename...>
    inline static const auto inner = identifier++;

public:

    /*! @brief Statically generated unique identifier for the given type. */
    template<typename... Type>
    inline static const uint32_t id = inner<std::decay_t<Type>...>;
};

template <typename Resource>
uint32_t getResourceTypeID() {
    return ResourceTypeID<Resource>::template id<Resource>;
}

namespace Progression { namespace ResourceManager {

    extern std::vector<std::unordered_map<std::string, Resource*>> resources;

    void init();
    //void update();
    //void join();
    void shutdown();

    template <typename T>
    T* get(const std::string& name) {
        LOG("name group = ", getResourceTypeID<T>(), "res size: ", resource.size());
        auto& group = resources[getResourceTypeID<T>()];
        auto it = group.find(name);
        LOG("got it");
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
    template <typename T>
    void add(const std::string& name, T* resource) {}

    // bool loadResourceFile(const std::string& fname);

} } // namespace Progression::Resource
