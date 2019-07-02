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
            db.clear();
            db.resize(TOTAL_RESOURCE_TYPES);
        }

        #define PARSE_AND_LOAD(TYPE) \
        else if (line == #TYPE) \
        { \
            TYPE resource; \
            if (!resource.loadFromResourceFile(in)) { \
                LOG_ERR("Failed to load resource with name '", resource.name, "'"); \
                return false; \
            } \
            \
            ResourceMap& resMap = DB[getResourceTypeID<TYPE>()]; \
            if (resMap.find(resource.name) != resMap.end()) { \
                *(resMap[resource.name]) = std::move(resource); \
            } else { \
                std::string name = resource.name; \
                resMap[name] = std::make_shared<TYPE>(std::move(resource)); \
            } \
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

            while (!bg_scanningThreadExit_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));

                // update file timestamps
                bool updateNeeded = false;
                for (auto& [name, data] : resourceFiles_) {
                    if (data.update()) {
                        LOG("file: ", name, " was out of date");
                        if (!BG_LoadResources(name, &bg_scanningResources_))
                            LOG("Could not reload the resource file: ", name);
                    }
                }

                for (size_t typeID = 0; typeID < f_resources.size() && !bg_scanningThreadExit_; ++typeID) {
                    auto& bg_map = bg_scanningResources_[typeID];
                    for (const auto& [resName, resPtr] : f_resources[typeID]) {
                        if (bg_scanningThreadExit_)
                            break;
                        auto newRes = resPtr->needsReloading();
                        if (newRes) {
                            LOG("Reload needed for resource: ", resName);
                            if (!newRes->load()) {
                                LOG_ERR("Failed to reload shader: ", resName);
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

    void moveBackgroundResources(ResourceDB& newResources) {
        for (size_t typeID = 0; typeID < TOTAL_RESOURCE_TYPES; ++typeID) {
            ResourceMap& currentResources = f_resources[typeID];
            for (auto& [name, res] : newResources[typeID]) {
                auto it = currentResources.find(name);
                if (it == currentResources.end()) {
                    currentResources[name] = res;
                } else {
                    res->move(it->second.get());
                    it->second = res;
                }
            }
        }
        resolveSoftLinks(newResources);
    }

    void update() {
        if (!bg_lock_.try_lock())
            return;

        moveBackgroundResources(bg_scanningResources_);
        bg_scanningResources_.clear();
        bg_scanningResources_.resize(TOTAL_RESOURCE_TYPES);
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
            LOG("Resolving mat: ", name);
            ret = ret && resolveMaterialTextures((Material*) res.get());
        }

        typeID = getResourceTypeID<Model>();
        for (auto& [name, res] : db[typeID]) {
            LOG("starting resolve for model: ", name);
            //Model* model = (Model*) res.get();
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
            LOG("Uploading meshes for model: ", name);

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
                    moveBackgroundResources(*data.db);
                    resourceFiles_[data.resFile.filename] = data.resFile;
                } else {
                    freeResources(*(data.db));
                    ret = false;
                }
                delete data.db;
            }
        }
        loadingResourceFiles_.clear();

        //resolveSoftLinks();

        return ret;
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

    bool createFastFile(const std::string& resourceFile) {
        shutdown();
        init(false);
        loadResourceFileAsync(resourceFile);
        if (!waitUntilLoadComplete()) {
            LOG_ERR("Failed to properly load all of the resource");
            return false;
        }

        return true;
    }

} } // namespace Progression::ResourceManager
