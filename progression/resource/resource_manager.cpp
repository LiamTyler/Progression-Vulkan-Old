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



namespace Progression {
    
namespace ResourceManager {

    using FileMap = std::unordered_map<std::string, TimeStampedFile>;
    
    ResourceDB f_resources;

    namespace {

        struct BG_LoaderData {
            BG_LoaderData() {
                db.resize(TOTAL_RESOURCE_TYPES);
                updateDB.resize(TOTAL_RESOURCE_TYPES);
            }

            std::future<bool> retVal;
            TimeStampedFile resFile;
            ResourceDB db;
            UpdateDB updateDB;
        };

        std::unordered_map<std::string, TimeStampedFile> resourceFiles_;
        std::unordered_map<std::string, struct BG_LoaderData> loadingResourceFiles_;

        std::thread bg_scanningThread_;
        ResourceDB bg_scanningResources_;
        UpdateDB bg_scanningUpdateDB_;
        bool bg_scanningThreadExit_;
        std::mutex bg_lock_;

        void freeResources(ResourceDB& db) {
            db.clear();
            db.resize(TOTAL_RESOURCE_TYPES);
        }

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
                ResourceMap& resMap = DB[getResourceTypeID<TYPE>()]; \
                if (resMap.find(resource.name) != resMap.end()) { \
                    *(resMap[resource.name]) = std::move(resource); \
                } else { \
                    std::string name = resource.name; \
                    resMap[name] = std::make_shared<TYPE>(std::move(resource)); \
                } \
            } else { \
                LOG("Adding resource func for res: ", resource.name); \
                updateDB[getResourceTypeID<TYPE>()][resource.name] = updateFunc; \
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
            bg_scanningResources_.resize(TOTAL_RESOURCE_TYPES);
            bg_scanningUpdateDB_.resize(TOTAL_RESOURCE_TYPES);

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

                for (size_t typeID = 0; typeID < f_resources.size() && !bg_scanningThreadExit_; ++typeID) {
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
            freeResources(bg_scanningResources_);
            bg_scanningResources_.clear();
        }

    } // namespace anonymous

    void init(bool scanner) {
        f_resources.resize(TOTAL_RESOURCE_TYPES);
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
        bg_scanningResources_.resize(TOTAL_RESOURCE_TYPES);
        bg_scanningUpdateDB_.clear();
        bg_scanningUpdateDB_.resize(TOTAL_RESOURCE_TYPES);
        bg_lock_.unlock();
    }

    bool resolveSoftLinks(ResourceDB& db) {
        auto resolveMaterialTextures = [](Material* mat) {
            auto& texName = mat->map_Kd_name;
            if (texName != "") {
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
                    freeResources(data.db);
                    ret = false;
                }
            }
        }
        loadingResourceFiles_.clear();

        //resolveSoftLinks();

        return ret;
    }

    void shutdown() {
        for (auto& [name, data]: loadingResourceFiles_) {
            data.retVal.wait();
            freeResources(data.db);
        }
        loadingResourceFiles_.clear();

        bg_scanningThreadExit_ = true;
        bg_scanningThread_.join();
        freeResources(f_resources);
        resourceFiles_.clear();
        f_resources.clear();
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

        struct BG_LoaderData& data = loadingResourceFiles_[fname];
        //data.db = new ResourceDB;
        data.db.resize(TOTAL_RESOURCE_TYPES);
        data.updateDB.resize(TOTAL_RESOURCE_TYPES);
        data.retVal = std::async(std::launch::async, BG_LoadResources, fname, &data.db, &data.updateDB);
        data.resFile = TimeStampedFile(fname);
    }

    #define PARSE_THEN_LOAD(TYPE) \
        else if (line == #TYPE) \
        { \
            TYPE resource; \
            if (!resource.readMetaData(in)) { \
                LOG("Could not parse metaData for resource: ", resource.name); \
                return false; \
            } \
            if (!resource.load()) { \
                LOG("Could not load resource: ", resource.name); \
                return false; \
            } \
            std::string name = resource.name; \
            DB[getResourceTypeID<TYPE>()][name] = std::make_shared<TYPE>(std::move(resource)); \
        }

    // TODO: factor out the parsing and loading of a resource file into two steps to avoid code duplication
    // with BG_LoadResources.
    bool createFastFile(const std::string& resourceFile) {
        ResourceDB DB;
        DB.resize(TOTAL_RESOURCE_TYPES);
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
            else {
                LOG_WARN("Unrecognized line: ", line);
            }
        }

        in.close();

        if (!resolveSoftLinks(DB)) {
            LOG("Could not resolve the soft links while parsing resource file: ", resourceFile);
            return false;
        }

        std::string fastFileName = resourceFile + ".ff";
        std::ofstream out(fastFileName, std::ios::binary);
        // for (size_t typeID = 0; typeID < TOTAL_RESOURCE_TYPES; ++typeID) {
        //     out << typeID << DB[typeID].size();
        // }
        ResourceMap& shaders = DB[getResourceTypeID<Shader>()];
        uint32_t numShaders = shaders.size();
        out.write((char*) &numShaders, sizeof(uint32_t));
        LOG("Num shaders: ", numShaders);
        for (const auto& [name, shader] : shaders) {
            LOG("writing shader: ", name);
            if (!shader->saveToFastFile(out)) {
                LOG("Could not write shader: ", shader->name, " to fast file");
                return false;
            }
        }

        //out.close();
        return true;
    }

    bool loadFastFile(const std::string& fastFile) {
        std::ifstream in(fastFile, std::ios::binary);
        if (!in) {
            LOG_ERR("Could not open fastfile:", fastFile);
            return false;
        }

        // std::vector<size_t> numTypes(TOTAL_RESOURCE_TYPES); 

        // for (size_t typeID = 0; typeID < TOTAL_RESOURCE_TYPES; ++typeID) {
        //     in >> numTypes[typeID];
        // }
        uint32_t numShaders;
        in.read((char*) &numShaders, sizeof(uint32_t));
        ResourceMap& shaders = f_resources[getResourceTypeID<Shader>()];
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

        in.close();
        return true;
    }

} } // namespace Progression::ResourceManager
