#include "resource/resource_manager.hpp"
#include "resource/mesh.hpp"
#include "core/configuration.hpp"
#include "resource/resourceIO/io.hpp"
#include "utils/logger.hpp"
#include <sys/stat.h>
#include <thread>
#include <mutex>
#include <future>
#include "core/window.hpp"

namespace Progression {
    
namespace ResourceManager {

    using ResourceMap = std::unordered_map<std::string, Resource*>;
    using ResourceDB = std::vector<ResourceMap>;
    using FileMap = std::unordered_map<std::string, TimeStampedFile>;
    
    ResourceDB f_resources;

    namespace {

        struct BG_LoaderData {
            std::future<bool> retVal;
            ResourceDB* db;
            TimeStampedFile resFile;
        };

        std::unordered_map<std::string, TimeStampedFile> resourceFiles_;
        std::unordered_map<std::string, struct BG_LoaderData> loadingResourceFiles_;

        std::thread bg_scanningThread_;
        ResourceDB bg_scanningResources_;
        bool bg_scanningThreadExit_;
        std::mutex bg_lock_;

        void freeResources(ResourceDB& db) {
            for (const auto& resourceMap : db) {
                for (auto& resourcePair : resourceMap) {
                    if (resourcePair.second)
                        delete resourcePair.second;
                }
            }
            db.clear();
        }

        Texture2D* loadTexture2DInternal(const std::string &name, const TextureMetaData &metaData, std::unordered_map<std::string, Resource*>& resources) {
            Texture2D tex(name, metaData);
            if (!tex.load())
            {
                LOG_ERR("Failed to load texture: ", name);
                return nullptr;
            }

            if (resources.find(name) != resources.end()) {
                *resources[name] = std::move(tex);
            } else {
                resources[name] = new Texture2D(std::move(tex));
            }

            return (Texture2D *)resources[name];
        }

        Shader* loadShaderInternal(const std::string &name, const ShaderMetaData &metaData, std::unordered_map<std::string, Resource*>& shaders) {
            Shader shader(name, metaData);
            if (!shader.load())
            {
                LOG_ERR("Failed to load shader: ", name);
                return nullptr;
            }

            if (shaders.find(name) != shaders.end()) {
                *shaders[name] = std::move(shader);
            } else {
                shaders[name] = new Shader(std::move(shader));
            }

            return (Shader *)shaders[name];
        }

        bool BG_LoadResources(const std::string& fname, ResourceDB* db) {
            auto& DB = *db;
            Window window;
            WindowCreateInfo createInfo;
            createInfo.width = 1;
            createInfo.height = 1;
            createInfo.visible = false;
            window.init(createInfo);
            window.bindContext();

            std::ifstream in(fname);
            if (!in) {
                LOG_ERR("Could not open resource file:", fname);
                return false;
            }

            std::string line;
            while (std::getline(in, line)) {
                if (line == "" || line[0] == '#'){
                    continue;
                } else if (line == "Shader") {
                    Shader shader;
                    if (!shader.loadMetaDataFromFile(in)) {
                        LOG("could not parse shader: ", shader.name);
                        return false;
                    }
                    LOG("parsed shader meta");
                    if (!loadShaderInternal(shader.name, shader.metaData, DB[getResourceTypeID<Shader>()])) {
                        LOG("Could not load shader: ", shader.name);
                        return false;
                    }
                } else if (line == "Texture2D") {
                    Texture2D tex;
                    if (!tex.loadMetaDataFromFile(in)) {
                        LOG("could not parse texture2D: ", tex.name);
                        return false;
                    }
                    LOG("parsed shader meta");
                    if (!loadTexture2DInternal(tex.name, tex.metaData, DB[getResourceTypeID<Texture2D>()])) {
                        LOG("Could not load texture2D: ", tex.name);
                        return false;
                    }
                }
            }

            in.close();

            return true;
        }

        void scanResources() {
            Window window;
            WindowCreateInfo createInfo;
            createInfo.width = 1;
            createInfo.height = 1;
            createInfo.visible = false;
            window.init(createInfo);
            window.bindContext();
            bg_scanningResources_.resize(TOTAL_RESOURCE_TYPES);

            while (!bg_scanningThreadExit_) {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                // update file timestamps
                bool updateNeeded = false;
                for (auto& [name, data] : resourceFiles_) {
                    if (data.update()) {
                        LOG("file: ", name, " was out of date");
                        if (!BG_LoadResources(name, &bg_scanningResources_))
                            LOG("Could not reload the resource file: ", name);
                    }
                }

                uint32_t typeID = -1;
                // shaders
                typeID = getResourceTypeID<Shader>();
                for (const auto& [name, res]: f_resources[typeID]) {
                    Shader* shader = (Shader*) res;

                    auto newShader = shader->needsReloading();
                    if (newShader) {
                        LOG("Reload needed for shader: ", name);
                        if (!newShader->load()) {
                            LOG_ERR("Failed to reload shader: ", name);
                            delete newShader;
                            continue;
                        }
                        bg_lock_.lock();
                        bg_scanningResources_[typeID][newShader->name] = newShader;
                        bg_lock_.unlock();
                    }
                }

                // texture2D
                typeID = getResourceTypeID<Texture2D>();
                for (const auto& [name, resource]: f_resources[typeID]) {
                    Texture2D* shader = (Texture2D*) resource;

                    auto newResource = shader->needsReloading();
                    if (newResource) {
                        LOG("Reload needed for texture2D: ", name);
                        if (!newResource->load()) {
                            LOG_ERR("Failed to reload texture2D: ", name);
                            delete newResource;
                            continue;
                        }
                        bg_lock_.lock();
                        bg_scanningResources_[typeID][newResource->name] = newResource;
                        bg_lock_.unlock();
                    }
                }
            }

            // exiting, clean up resources
            freeResources(bg_scanningResources_);
            bg_scanningResources_.clear();
        }

    } // namespace anonymous

    void init() {
        f_resources.resize(TOTAL_RESOURCE_TYPES);
        bg_scanningThreadExit_ = false;
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // materials_["default"] = Material();

        bg_scanningThread_ = std::thread(scanResources);
    }

    void moveBackgroundResources(ResourceDB& newResources) {
        uint32_t typeID = getResourceTypeID<Shader>();
        ResourceMap* currentResources = &f_resources[typeID];
        for (auto& [name, res] : newResources[typeID]) {
            auto it = currentResources->find(name);
            if (it == currentResources->end()) {
                (*currentResources)[name] = res;
            } else {
                *(Shader*) it->second = std::move(*(Shader*) res);
                delete res;
            }
        }

        typeID = getResourceTypeID<Texture2D>();
        currentResources = &f_resources[typeID];
        for (auto& [name, res] : newResources[typeID]) {
            auto it = currentResources->find(name);
            if (it == currentResources->end()) {
                (*currentResources)[name] = res;
            } else {
                *(Texture2D*) it->second = std::move(*(Texture2D*) res);
                delete res;
            }
        }
    }

    void update() {
        if (!bg_lock_.try_lock())
            return;

        moveBackgroundResources(bg_scanningResources_);
        bg_scanningResources_.clear();
        bg_scanningResources_.resize(TOTAL_RESOURCE_TYPES);
        bg_lock_.unlock();
    }

    void waitUntilLoadComplete(const std::string& resFile) {
        for (auto& [name, data] : loadingResourceFiles_) {
            if (resFile == "" || name == resFile) {
                data.retVal.wait();
                if (data.retVal.get()) {
                    moveBackgroundResources(*data.db);
                    resourceFiles_[data.resFile.filename] = data.resFile;
                } else {
                    freeResources(*(data.db));
                }
                delete data.db;
            }
        }
        loadingResourceFiles_.clear();
    }

    void shutdown() {
        for (auto& [name, data]: loadingResourceFiles_) {
            data.retVal.wait();
            freeResources(*(data.db));
            delete data.db;
        }
        loadingResourceFiles_.clear();

        bg_scanningThreadExit_ = true;
        bg_scanningThread_.join();
        freeResources(f_resources);
        resourceFiles_.clear();
        f_resources.clear();
    }

    Texture2D* loadTexture2D(const std::string& name, const TextureMetaData& metaData) {
        return loadTexture2DInternal(name, metaData, f_resources[getResourceTypeID<Texture2D>()]);
    }

    Shader* loadShader(const std::string& name, const ShaderMetaData& metaData) {
        return loadShaderInternal(name, metaData, f_resources[getResourceTypeID<Shader>()]);
    }

    void loadResourceFileAsync(const std::string& fname) {
        // if alreading being processed, ignore
        if (loadingResourceFiles_.find(fname) != loadingResourceFiles_.end())
            return;
        
        // ignore if file is up to date
        auto it = resourceFiles_.find(fname);
        if (it != resourceFiles_.end()) {
            auto newTimestamp = TimeStampedFile(fname);
            if (!it->second.outOfDate(newTimestamp))
                return;
        }

        struct BG_LoaderData data;
        data.db = new ResourceDB;
        data.db->resize(TOTAL_RESOURCE_TYPES);
        data.retVal = std::async(std::launch::async, BG_LoadResources, fname, data.db);
        data.resFile = TimeStampedFile(fname);
        loadingResourceFiles_[fname] = std::move(data);
    }

} } // namespace Progression::ResourceManager