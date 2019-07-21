#include "resource/resource_manager.hpp"
#include "resource/model.hpp"
#include "resource/material.hpp"
#include "resource/shader.hpp"
#include "resource/texture2D.hpp"
#include "core/configuration.hpp"
#include <sys/stat.h>
#include "utils/logger.hpp"
#include <thread>
#include <mutex>
#include <future>
#include <tuple>
#include "core/window.hpp"
#include "utils/serialize.hpp"

namespace Progression {
    
namespace ResourceManager {

    using FileMap = std::unordered_map<std::string, TimeStampedFile>;
    
    ResourceDB f_resources;

    namespace {

        typedef struct BG_LoaderData {
            std::future<bool> retVal;
            TimeStampedFile resFile;
            ResourceDB db;
            UpdateDB updateDB;
        } BG_LoaderData;

        std::unordered_map<std::string, TimeStampedFile> resourceFiles_;
        std::unordered_map<std::string, struct BG_LoaderData> loadingResourceFiles_;

        std::thread bg_scanningThread_;
        ResourceDB bg_scanningResources_;
        UpdateDB bg_scanningUpdateDB_;
        bool bg_scanningThreadExit_;
        std::mutex bg_lock_;

        #define PARSE_AND_LOAD(TYPE) \
        else if (line == #TYPE) \
        { \
            std::function<void()> updateFunc; \
            TYPE resource; \
            ResUpdateStatus status = resource.loadFromResourceFile(in, updateFunc); \
            if (status == RES_UP_TO_DATE) { \
                continue; \
            } else if (status == RES_PARSE_ERROR) { \
                LOG("Error while parsing resource: ", resource.name); \
                return false; \
            } else if (status == RES_RELOAD_FAILED) { \
                LOG("Reload failed for resource: ", resource.name); \
                return false; \
            } else if (status == RES_RELOAD_SUCCESS) { \
                /* TODO: is this check for in the DB even needed? */ \
                ResourceMap& resMap = DB.getMap<TYPE>(); \
                if (resMap.find(resource.name) != resMap.end()) { \
                    resource.move(resMap[resource.name].get()); \
                } else { \
                    std::string name = resource.name; \
                    resMap[name] = std::make_shared<TYPE>(std::move(resource)); \
                } \
            } else { \
                LOG("Adding resource func for res: ", resource.name); \
                updateDB.getMap<TYPE>()[resource.name] = updateFunc; \
            } \
        }

        bool BG_LoadResources(const std::string& fname, ResourceDB* db, UpdateDB* _updateDB) {
            auto& DB = *db;
            auto& updateDB = *_updateDB;
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
            while (std::getline(in, line) && !bg_scanningThreadExit_) {
                if (line == "" || line[0] == '#') {
                    continue;
                }
                PARSE_AND_LOAD(Shader)
                PARSE_AND_LOAD(Texture2D)
                PARSE_AND_LOAD(Material)
                PARSE_AND_LOAD(Model)
                else {
                    LOG_WARN("Unrecognized line: ", line);
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

            while (!bg_scanningThreadExit_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // update file timestamps
                bool updateNeeded = false;
                for (auto& [name, data] : resourceFiles_) {
                    if (data.update()) {
                        LOG("file: ", name, " was out of date");
                        bg_lock_.lock();
                        if (!BG_LoadResources(name, &bg_scanningResources_, &bg_scanningUpdateDB_))
                            LOG("Could not reload the resource file: ", name);
                        bg_lock_.unlock();
                    }
                }

                for (size_t typeID = 0; typeID < TOTAL_RESOURCE_TYPES && !bg_scanningThreadExit_; ++typeID) {
                    auto& bg_map = bg_scanningResources_[typeID];
                    for (const auto& [resName, resPtr] : f_resources[typeID]) {
                        if (bg_scanningThreadExit_)
                            break;
                        auto newRes = resPtr->needsReloading();
                        if (newRes) {
                            if (!newRes->load()) {
                                LOG_ERR("Failed to reload resource: ", resName);
                                continue;
                            }
                            bg_lock_.lock();
                            bg_map[resName] = newRes;
                            bg_lock_.unlock();
                        }
                    }
                }
            }
            // exiting, clean up resources
            bg_scanningResources_.clear();
        }

    } // namespace anonymous

    void init(bool scanner) {
        bg_scanningThreadExit_ = false;
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        auto defaultMat = std::make_shared<Material>();
        defaultMat->Kd = glm::vec3(1, 1, 0);
        f_resources[getResourceTypeID<Material>()]["default"] = defaultMat;
        if (scanner)
            bg_scanningThread_ = std::thread(scanResources);
    }

    void moveBackgroundData(ResourceDB& newResources, UpdateDB& newUpdates) {
        for (size_t typeID = 0; typeID < TOTAL_RESOURCE_TYPES; ++typeID) {
            ResourceMap& currentResources = f_resources[typeID];
            for (auto& [name, res] : newResources[typeID]) {
                auto it = currentResources.find(name);
                if (it == currentResources.end()) {
                    currentResources[name] = res;
                } else {
                    res->move(it->second.get());
                    res = it->second;
                }
            }

            for (auto& [name, updateFunc] : newUpdates[typeID]) {
                updateFunc();
            }
        }
        
        resolveSoftLinks(newResources);
    }

    void update() {
        if (!bg_lock_.try_lock())
            return;

        moveBackgroundData(bg_scanningResources_, bg_scanningUpdateDB_);
        bg_scanningResources_.clear();
        bg_scanningUpdateDB_.clear();
        bg_lock_.unlock();
    }

    bool resolveSoftLinks(ResourceDB& db) {
        auto resolveMaterialTextures = [](Material* mat) {
            auto& texName = mat->map_Kd_name;
            if (texName != "" && !mat->map_Kd) {
                if (ResourceManager::get<Texture2D>(texName)) {
                    mat->map_Kd = ResourceManager::get<Texture2D>(texName).get();
                } else {
                    TextureMetaData data;
                    data.file = TimeStampedFile(PG_RESOURCE_DIR + texName);
                    mat->map_Kd = ResourceManager::load<Texture2D>(texName, &data).get();
                    if (!mat->map_Kd)
                        return false;
                }
            }
            return true;
        };

        bool ret = true;
        auto typeID = getResourceTypeID<Material>();
        for (auto& [name, res] : db[typeID]) {
            ret = ret && resolveMaterialTextures((Material*) res.get());
        }

        typeID = getResourceTypeID<Model>();
        for (auto& [name, res] : db[typeID]) {
            std::shared_ptr<Model> model = std::static_pointer_cast<Model>(res);
            for (size_t i = 0; i < model->materials.size(); ++i) {
                auto& mat = model->materials[i];
                if (get<Material>(mat->name)) {
                    model->materials[i] = get<Material>(mat->name);
                    continue;
                }

                // TODO (?) add to manager
                ret = ret && resolveMaterialTextures(mat.get());
            }

            for (auto& mesh : model->meshes) {
                mesh.uploadToGpu(model->metaData.freeCpuCopy);
            }
        }

        return ret;
    }

    bool waitUntilLoadComplete(const std::string& resFile) {\
        bool ret = true;
        for (auto& [name, data] : loadingResourceFiles_) {
            if (resFile == "" || name == resFile) {
                data.retVal.wait();
                if (data.retVal.get()) {
                    moveBackgroundData(data.db, data.updateDB);
                    resourceFiles_[data.resFile.filename] = data.resFile;
                } else {
                    ret = false;
                }
            }
        }
        loadingResourceFiles_.clear();

        return ret;
    }

    void shutdown() {
        for (auto& [name, data]: loadingResourceFiles_) {
            data.retVal.wait();
        }
        loadingResourceFiles_.clear();

        bg_scanningThreadExit_ = true;
        bg_scanningThread_.join();
        resourceFiles_.clear();
        f_resources.clear();
    }

    #define PARSE_THEN_LOAD(TYPE) \
        else if (line == #TYPE) \
        { \
            auto resource = std::make_shared<TYPE>(); \
            if (!resource->readMetaData(in)) { \
                LOG("Could not parse metaData for resource: ", resource->name); \
                return false; \
            } \
            if (!resource->load()) { \
                LOG("Could not load resource: ", resource->name); \
                return false; \
            } \
            DB.getMap<TYPE>()[resource->name] = resource; \
        }

    // TODO: factor out the parsing and loading of a resource file into two steps to avoid code duplication
    // with BG_LoadResources.
    bool createFastFile(const std::string& resourceFile) {
        ResourceDB DB;
        std::ifstream in(resourceFile);
        if (!in) {
            LOG_ERR("Could not open resource file:", resourceFile);
            return false;
        }

        std::string line;
        while (std::getline(in, line)) {
            if (line == "" || line[0] == '#') {
                continue;
            }
            PARSE_THEN_LOAD(Shader)
            PARSE_THEN_LOAD(Material)
            PARSE_THEN_LOAD(Model)
            PARSE_THEN_LOAD(Texture2D)
            else {
                LOG_WARN("Unrecognized line: ", line);
            }
        }

        in.close();

        std::string fastFileName = resourceFile + ".ff";
        std::ofstream out(fastFileName, std::ios::binary);
        ResourceMap& shaders = DB.getMap<Shader>();
        uint32_t numShaders = shaders.size();
        serialize::write(out, numShaders);
        for (const auto& [name, shader] : shaders) {
            if (!shader->saveToFastFile(out)) {
                LOG("Could not write shader: ", shader->name, " to fast file");
                return false;
            }
        }

        ResourceMap& materials = DB.getMap<Material>();
        uint32_t numMaterials = materials.size();
        serialize::write(out, numMaterials);
        for (const auto& [name, material] : materials) {
            if (!material->saveToFastFile(out)) {
                LOG("Could not write material: ", material->name, " to fast file");
                return false;
            }
        }

        ResourceMap& models = DB.getMap<Model>();
        uint32_t numModels = models.size();
        serialize::write(out, numModels);
        for (const auto& [name, model] : models) {
            if (!model->saveToFastFile(out)) {
                LOG("Could not write model: ", model->name, " to fast file");
                return false;
            }
        }

        ResourceMap& textures = DB.getMap<Texture2D>();
        uint32_t numTextures = textures.size();
        serialize::write(out, numTextures);
        for (const auto& [name, texture] : textures) {
            if (!texture->saveToFastFile(out)) {
                LOG("Could not write texture2D: ", texture->name, " to fast file");
                return false;
            }
        }

        //out.close();
        return true;
    }

    static void loadResourceFileAsync(const std::string& fname) {
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

        BG_LoaderData& data = loadingResourceFiles_[fname];
        data.retVal = std::async(std::launch::async, BG_LoadResources, fname, &data.db, &data.updateDB);
        data.resFile = TimeStampedFile(fname);
    }

    static bool loadFastFile(const std::string& fastFile) {
        std::ifstream in(fastFile, std::ios::binary);
        if (!in) {
            LOG_ERR("Could not open fastfile:", fastFile);
            return false;
        }

        ResourceDB DB;

        uint32_t numShaders;
        serialize::read(in, numShaders);
        ResourceMap& shaders = DB.getMap<Shader>();
        for (uint32_t i = 0; i < numShaders; ++i) {
            auto shader = std::make_shared<Shader>();
            if (!shader->loadFromFastFile(in)) {
                LOG_ERR("Failed to load shader from fastfile");
                return false;
            }
            if (shaders.find(shader->name) != shaders.end()) {
                shader->move(shaders[shader->name].get());
            } else {
                shaders[shader->name] = shader;
            }
        }

        uint32_t numMaterials;
        serialize::read(in, numMaterials);
        ResourceMap& materials = DB.getMap<Material>();
        for (uint32_t i = 0; i < numMaterials; ++i) {
            auto material = std::make_shared<Material>();
            if (!material->loadFromFastFile(in)) {
                LOG_ERR("Failed to load material from fastfile");
                return false;
            }
            if (materials.find(material->name) != materials.end()) {
                material->move(materials[material->name].get());
            } else {
                materials[material->name] = material;
            }
        }

        uint32_t numModels;
        serialize::read(in, numModels);
        ResourceMap& models = DB.getMap<Model>();
        for (uint32_t i = 0; i < numModels; ++i) {
            auto model = std::make_shared<Model>();
            if (!model->loadFromFastFile(in)) {
                LOG_ERR("Failed to load model from fastfile");
                return false;
            }
            if (models.find(model->name) != models.end()) {
                model->move(models[model->name].get());
            } else {
                models[model->name] = model;
            }
        }

        uint32_t numTextures;
        serialize::read(in, numTextures);
        ResourceMap& textures = DB.getMap<Texture2D>();
        for (uint32_t i = 0; i < numTextures; ++i) {
            auto texture = std::make_shared<Texture2D>();
            if (!texture->loadFromFastFile(in)) {
                LOG_ERR("Failed to load texture from fastfile");
                return false;
            }
            if (textures.find(texture->name) != textures.end()) {
                texture->move(textures[texture->name].get());
            } else {
                textures[texture->name] = texture;
            }
        }

        in.close();

        UpdateDB updateDB;
        moveBackgroundData(DB, updateDB);
        return true;
    }

    bool loadResourceFile(const std::string& fname) {
        auto lastDot = fname.find_last_of('.');
        if (lastDot == std::string::npos) {
            LOG_ERR("No file format recognized on resource file: ", fname);
            return false;
        }
        std::string ext = fname.substr(lastDot);
        if (ext == ".ff") {
            return loadFastFile(fname);
        } else if (ext == ".txt") {
            loadResourceFileAsync(fname);
            return true;
        } else {
            LOG_ERR("Did not find '.txt' or '.ff' extension on resource file: ", fname);
            return false;
        }
    }

} } // namespace Progression::ResourceManager
