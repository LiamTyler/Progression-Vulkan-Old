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

    using ResourceDB = std::vector<std::unordered_map<std::string, Resource*>>;
    using FileMap = std::unordered_map<std::string, TimeStampedFile>;
    
    ResourceDB resources;

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

        void freeResources(ResourceDB* db) {
            for (const auto& resourceMap : *db) {
                for (auto& resourcePair : resourceMap) {
                    if (resourcePair.second)
                        delete resourcePair.second;
                }
            }
        }

        bool loadShaderMetaFromFile(std::istream& in, std::string& name, ShaderMetaData& metaData) {
            std::string line;
            std::string s;
            std::istringstream ss;

            std::getline(in, line);
            ss = std::istringstream(line);
            ss >> s;
            PG_ASSERT(s == "name");
            ss >> name;
            PG_ASSERT(!in.fail() && !ss.fail());

            std::getline(in, line);
            ss = std::istringstream(line);
            ss >> s;
            PG_ASSERT(s == "vertex");
            if (!ss.eof()) {
                ss >> s;
                metaData.vertex = TimeStampedFile(PG_RESOURCE_DIR + s);
            }
            PG_ASSERT(!in.fail() && !ss.fail());

            std::getline(in, line);
            ss = std::istringstream(line);
            ss >> s;
            PG_ASSERT(s == "geometry");
            if (!ss.eof()) {
                ss >> s;
                metaData.geometry = TimeStampedFile(PG_RESOURCE_DIR + s);
            }
            PG_ASSERT(!in.fail() && !ss.fail());

            std::getline(in, line);
            ss = std::istringstream(line);
            ss >> s;
            PG_ASSERT(s == "fragment");
            if (!ss.eof()) {
                ss >> s;
                metaData.fragment = TimeStampedFile(PG_RESOURCE_DIR + s);
            }
            PG_ASSERT(!in.fail() && !ss.fail());

            std::getline(in, line);
            ss = std::istringstream(line);
            ss >> s;
            PG_ASSERT(s == "compute");
            if (!ss.eof()) {
                ss >> s;
                metaData.compute = TimeStampedFile(PG_RESOURCE_DIR + s);
            }
            PG_ASSERT(!in.fail() && !ss.fail());

            return true;
        }

        Shader* loadShaderInternal(const std::string &name, const ShaderMetaData &metaData, std::unordered_map<std::string, Resource*>& shaders) {
            Shader shader(name, metaData);
            if (!shader.loadFromText())
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
                if (line == "")
                    continue;
                if (line[0] == '#')
                    continue;
                if (line == "Shader") {
                    std::string name;
                    ShaderMetaData metaData;
                    if (!loadShaderMetaFromFile(in, name, metaData)) {
                        LOG("could not parse shader: ", name);
                        return false;
                    }
                    LOG("parsed shader meta");
                    if (!loadShaderInternal(name, metaData, DB[getResourceTypeID<Shader>()])) {
                        LOG("Could not load shader: ", name);
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
                        updateNeeded = true;
                    }
                }

                if (!updateNeeded)
                    continue;

                // shaders
                // auto shaderID = getResourceTypeID<Shader>();
                // for (const auto& resourcePair : resources[shaderID]) {
                //     const auto& name = resourcePair.first;
                //     Shader* shader = (Shader*) resourcePair.second;
                //     ShaderMetaData newMetaData;
                //     newMetaData.vertex   = fileTimeStamps_[shader->metaData.vertex.filename];
                //     newMetaData.geometry = fileTimeStamps_[shader->metaData.geometry.filename];
                //     newMetaData.fragment = fileTimeStamps_[shader->metaData.fragment.filename];
                //     newMetaData.compute  = fileTimeStamps_[shader->metaData.compute.filename];

                //     if (shader->metaData.outOfDate(newMetaData)) {
                //         LOG("Reload needed for shader: ", name);
                //         Shader* newShader = new Shader(name, newMetaData);
                //         if (!newShader->loadFromText()) {
                //             LOG_ERR("Failed to reload shader: ", name);
                //             shader->metaData = newMetaData;
                //             continue;
                //         }
                //         backgroundResources_[shaderID][name] = newShader;
                //     }
                // }
            }

            // exiting, clean up resources
            freeResources(&bg_scanningResources_);
            bg_scanningResources_.clear();
        }

    } // namespace anonymous

    void init() {
        resources.resize(TOTAL_RESOURCE_TYPES);
        bg_scanningThreadExit_ = false;
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        // materials_["default"] = Material();

        bg_scanningThread_ = std::thread(scanResources);
    }

    void moveBackgroundResources(ResourceDB& newResources) {
        auto shaderID = getResourceTypeID<Shader>();
        auto& currentResources = resources[shaderID];
        for (auto& [name, res] : newResources[shaderID]) {
            auto it = currentResources.find(name);
            if (it == currentResources.end()) {
                currentResources[name] = res;
            } else {
                *(Shader*) it->second = std::move(*(Shader*) res);
                delete res;
            }
        }
    }

    void update() {
    }

    void waitUntilLoadComplete(const std::string& resFile) {
        for (auto& [name, data] : loadingResourceFiles_) {
            if (resFile == "" || name == resFile) {
                data.retVal.wait();
                if (data.retVal.get()) {
                    moveBackgroundResources(*data.db);
                    resourceFiles_[data.resFile.filename] = data.resFile;
                } else {
                    freeResources(data.db);
                }
                delete data.db;
            }
        }
        loadingResourceFiles_.clear();
    }

    void shutdown() {
        for (auto& [name, data]: loadingResourceFiles_) {
            data.retVal.wait();
            freeResources(data.db);
            delete data.db;
        }
        loadingResourceFiles_.clear();

        bg_scanningThreadExit_ = true;
        bg_scanningThread_.join();
        freeResources(&resources);
        resourceFiles_.clear();
        resources.clear();
    }

    Shader* loadShader(const std::string& name, const ShaderMetaData& metaData) {
        return loadShaderInternal(name, metaData, resources[getResourceTypeID<Shader>()]);
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


    // static std::unordered_map<std::string, GLint> internalFormatMap = {
    //     { "R8",      GL_R8 },
    //     { "RG8",     GL_RG},
    //     { "RGB8",    GL_RGB },
    //     { "RGBA8",   GL_RGBA },
    //     { "R16F",    GL_R16F },
    //     { "RG16F",   GL_RG16F },
    //     { "RGB16F",  GL_RGB16F },
    //     { "RGBA16F", GL_RGBA16F },
    //     { "R32F",    GL_R32F },
    //     { "RG32F",   GL_RG32F },
    //     { "RGB32F",  GL_RGB32F },
    //     { "RGBA32F", GL_RGBA32F },
    //     { "SRGB8",   GL_SRGB8 },
    //     { "SRGBA8",  GL_SRGB8_ALPHA8 },
    //     { "DEPTH",   GL_DEPTH_COMPONENT },
    // };

    // static std::unordered_map<std::string, GLint> minFilterMap = {
    //     { "nearest", GL_NEAREST },
    //     { "linear", GL_LINEAR },
    //     { "nearest_mipmap_nearest", GL_NEAREST_MIPMAP_NEAREST },
    //     { "linear_mipmap_nearest", GL_LINEAR_MIPMAP_NEAREST },
    //     { "nearest_mipmap_linear", GL_NEAREST_MIPMAP_LINEAR },
    //     { "linear_mipmap_linear", GL_LINEAR_MIPMAP_LINEAR },
    // };

    // static std::unordered_map<std::string, GLint> magFilterMap = {
    //     { "nearest", GL_NEAREST },
    //     { "linear", GL_LINEAR },
    // };

    // static std::unordered_map<std::string, GLint> wrapMap = {
    //     { "clamp_to_edge", GL_CLAMP_TO_EDGE },
    //     { "clamp_to_border", GL_CLAMP_TO_BORDER },
    //     { "mirror_repeat", GL_MIRRORED_REPEAT },
    //     { "repeat", GL_REPEAT },
    //     { "mirror_clamp_to_edge", GL_MIRROR_CLAMP_TO_EDGE },
    // };

    // bool loadTextureFromResourceFile(std::istream& in) {
    //     std::string line;
    //     std::string s;
    //     std::istringstream ss;
    //     std::unordered_map<std::string, GLint>::iterator it;
    //     std::string fname;
    //     TextureUsageDesc desc;
    //     bool freeCPUCopy;

    //     // texture name
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "filename");
    //     ss >> fname;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // freeCPUCopy
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "freeCPUCopy");
    //     ss >> s;
    //     PG_ASSERT(s == "true" || s == "false");
    //     freeCPUCopy = s == "true";
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // mipmaped
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "mipmapped");
    //     ss >> s;
    //     PG_ASSERT(s == "true" || s == "false");
    //     desc.mipMapped = s == "true";
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // internal format
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "internalFormat");
    //     ss >> s;
    //     it = internalFormatMap.find(s);
    //     if (it == internalFormatMap.end()) {
    //         LOG_ERR("Invalid texture format: ", s);
    //         return false;
    //     }
    //     desc.internalFormat = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());


    //     // min filter
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "minFilter");
    //     ss >> s;
    //     it = minFilterMap.find(s);
    //     if (it == minFilterMap.end()) {
    //         LOG_ERR("Invalid minFilter format: ", s);
    //         return false;
    //     }
    //     if (!desc.mipMapped && (s != "nearest" && s != "linear")) {
    //         LOG_WARN("Trying to use a mip map min filter when there is no mip map on texture: ", fname);
    //     }
    //     desc.minFilter = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // mag filter
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "magFilter");
    //     ss >> s;
    //     it = magFilterMap.find(s);
    //     if (it == magFilterMap.end()) {
    //         LOG_ERR("Invalid magFilter format: ", s);
    //         return false;
    //     }
    //     desc.magFilter = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // wrapModeS
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "wrapModeS");
    //     ss >> s;
    //     it = wrapMap.find(s);
    //     if (it == wrapMap.end()) {
    //         LOG_ERR("Invalid wrapModeS format: ", s);
    //         return false;
    //     }
    //     desc.wrapModeS = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     // wrapModeT
    //     std::getline(in, line);
    //     ss = std::istringstream(line);
    //     ss >> s;
    //     PG_ASSERT(s == "wrapModeT");
    //     ss >> s;
    //     it = wrapMap.find(s);
    //     if (it == wrapMap.end()) {
    //         LOG_ERR("Invalid wrapModeT format: ", s);
    //         return false;
    //     }
    //     desc.wrapModeT = it->second;
    //     PG_ASSERT(!in.fail() && !ss.fail());

    //     return Resource::loadTexture2D(fname, PG_RESOURCE_DIR + fname, desc, freeCPUCopy) != nullptr;
    // }
