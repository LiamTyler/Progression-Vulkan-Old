#include "resource/resource_manager.hpp"
#include "resource/mesh.hpp"
#include "core/configuration.hpp"
#include "resource/resourceIO/io.hpp"
#include "utils/logger.hpp"
#include <sys/stat.h>

typedef struct TimeStamp {
    TimeStamp() : valid(false) {}
    TimeStamp(const std::string& fname) {
        struct stat s;
        valid = stat(fname.c_str(), &s) == 0;
        if (valid)
            timestamp = s.st_mtim.tv_sec;
    }

    bool operator==(const TimeStamp& ts) const {
        return timestamp == ts.timestamp;
    }
    bool operator!=(const TimeStamp& ts) const {
        return !(*this == ts);
    }

    bool outOfDate(const TimeStamp& ts) const {
        return (!valid && ts.valid) || (valid && ts.valid && timestamp < ts.timestamp);
    }

    bool valid;
    time_t timestamp;
} TimeStamp;

namespace Progression {

    namespace {

        std::unordered_map<std::string, Model> models_;         // name = whatever specified
        std::unordered_map<std::string, Material> materials_;   // name = name as seen in mtl file
        std::unordered_map<std::string, Texture2D> textures2D_; // name = whatever specified 
        std::unordered_map<std::string, Shader> shaders_;       // name = whatever specified
        // std::unordered_map<std::string, Skybox> skyboxes_;   // name = whatever specified
        
        std::unordered_map<std::string, Shader> updatedShaders_;

        typedef struct ShaderTimeStamp {
            ShaderTimeStamp() {}
            ShaderTimeStamp(const ShaderFileDesc& d) :
                desc(d),
                vertex(TimeStamp(d.vertex)),
                geometry(TimeStamp(d.geometry)),
                fragment(TimeStamp(d.fragment)),
                compute(TimeStamp(d.compute))
            {
            }

            bool outOfDate(const ShaderTimeStamp& sts) {
                return vertex.outOfDate(sts.vertex) ||
                       geometry.outOfDate(sts.geometry) ||
                       fragment.outOfDate(sts.fragment) ||
                       compute.outOfDate(sts.compute);
            }
                    
            ShaderFileDesc desc;
            TimeStamp vertex;
            TimeStamp geometry;
            TimeStamp fragment;
            TimeStamp compute;
        } ShaderTimeStamp;

        std::unordered_map<std::string, TimeStamp> resourceFiles_;
        std::unordered_map<std::string, ShaderTimeStamp> shaderTimeStamps_;

        bool loadShaderFromResourceFile(std::istream& in);
    } // namespace anonymous

namespace Resource {

    void init() {
        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        materials_["default"] = Material();
    }

    void update() {
        for (const auto& pair : resourceFiles_) {
            const auto& fname = pair.first;
            const auto& savedTimeStamp = pair.second;
            TimeStamp currentTimeStamp(fname);
            // LOG(savedTimeStamp.timestamp.tv_sec, "  ", currentTimeStamp.timestamp.tv_sec);
            if (savedTimeStamp.outOfDate(currentTimeStamp)) {
                LOG("Need to reload the resource file: ", fname);
                if (!loadResourceFile(fname)) {
                    LOG_ERR("Could not reload the resource file during update");
                }
            }
        }
    }

    void join() {
        for (auto& pair : updatedShaders_) {
            const auto& name = pair.first;
            auto& shader = pair.second;
            shaders_[name] = std::move(shader);
        }
        updatedShaders_.clear();
    }

    void shutdown() {
        models_.clear();
        materials_.clear();
        textures2D_.clear();
        shaders_.clear();
    }

    Model* getModel(const std::string& name) {
        auto it = models_.find(name);
        return it == models_.end() ? nullptr : &it->second;
    }

    Material* getMaterial(const std::string& name) {
        auto it = materials_.find(name);
        return it == materials_.end() ? nullptr : &it->second;
    }

    Texture2D* getTexture2D(const std::string& name) {
        auto it = textures2D_.find(name);
        return it == textures2D_.end() ? nullptr : &it->second;
    }

    Shader* getShader(const std::string& name) {
        auto it = shaders_.find(name);
        return it == shaders_.end() ? nullptr : &it->second;
    }


    Model* loadModel(const std::string& name, const std::string& fname, bool optimize, bool freeCPUCopy) {
        if (getModel(name)) {
            LOG_WARN("Reloading model that is already loaded");
        }

        models_[name] = std::move(Model());
        if (!loadModelFromObj(models_[name], fname, optimize, freeCPUCopy)) {
            LOG("Could not load model file: ", fname);
            models_.erase(name);
            return nullptr;
        }
        return &models_[name];
    }

    std::vector<Material*> loadMaterials(const std::string& fname) {
        std::vector<std::pair<std::string, Material>> materials;
        if (!loadMtlFile(materials, fname, PG_RESOURCE_DIR)) {
            LOG("Could not load mtl file: ", fname);
            return {};
        }

        std::vector<Material*> ret;
        for (const auto& pair : materials) {
            if (materials_.find(pair.first) != materials_.end())
                LOG_WARN("Resource manager already contains material with name: ", pair.first);

            materials_[pair.first] = std::move(pair.second);
            ret.push_back(&materials_[pair.first]);
        }

        return ret;
    }

    Texture2D* loadTexture2D(const std::string& name, const std::string& fname, const TextureUsageDesc& desc, bool freeCPUCopy) {
        if (getTexture2D(name)) {
            LOG_WARN("Reloading texture that is already loaded");
        }

        if (!loadTexture2D(textures2D_[name], fname, desc, freeCPUCopy)) {
            LOG("Could not load texture file: ", fname);
            textures2D_.erase(name);
            return nullptr;
        }

        return &textures2D_[name];
    }

    Shader* loadShader(const std::string& name, const ShaderFileDesc& desc) {
        if (getShader(name)) {
            LOG_WARN("Reloading shader that is already loaded");
        }

        if (!loadShaderFromText(shaders_[name], desc)) {
            LOG("Could not load shader ", name);
            shaders_.erase(name);
            return nullptr;
        }

        return &shaders_[name];
    }

    bool loadResourceFile(const std::string& fname) {
        static int load = 0;
        load++;
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
            if (line == "Material") {
                if (load > 1)
                    continue;
                if (!loadMaterialFromResourceFile(in)) {
                    LOG_ERR("could not parse and create material");
                    return false;
                }
            } else if (line == "Model") {
                if (load > 1)
                    continue;
                if (!loadModelFromResourceFile(in)) {
                    LOG_ERR("could not parse and load model");
                    return false;
                }
            } else if (line == "Texture2D") {
                if (load > 1)
                    continue;
                if (!loadTextureFromResourceFile(in)) {
                    LOG_ERR("Could not parse and load texture");
                    return false;
                }
            } else if (line == "Shader") {
                if (!loadShaderFromResourceFile(in)) {
                    LOG("could not parse and load shader");
                    return false;
                }
            }
        }

        in.close();

        resourceFiles_[fname] = TimeStamp(fname);
        return true;
    }

    void addModel(const std::string& name, Model* model) {
        if (models_.find(name) != models_.end())
            LOG_WARN("Overriding model with name: ", name);
        models_[name] = std::move(*model);
    }

    void addMaterial(const std::string& name, Material* mat) {
        if (materials_.find(name) != materials_.end())
            LOG_WARN("Overriding material with name: ", name);
        materials_[name] = std::move(*mat);
    }

    void addTexture2D(const std::string& name, Texture2D* tex) {
        if (textures2D_.find(name) != textures2D_.end())
            LOG_WARN("Overriding texture2D with name: ", name);
        textures2D_[name] = std::move(*tex);
    }

    void addShader(const std::string& name, Shader* shader) {
        if (shaders_.find(name) != shaders_.end())
            LOG_WARN("Overriding shader with name: ", name);
        shaders_[name] = std::move(*shader);
    }

} } // namespace Progression::Resource

namespace Progression { namespace {

    bool loadShaderFromResourceFile(std::istream& in) {
        std::string line;
        std::string s;
        std::istringstream ss;
        std::string name;
        ShaderFileDesc desc;

        // shader name
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "name");
        ss >> name;
        PG_ASSERT(!in.fail() && !ss.fail());

        // vertex
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "vertex");
        if (!ss.eof())
            ss >> desc.vertex;
        PG_ASSERT(!in.fail() && !ss.fail());

        // geometry
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "geometry");
        if (!ss.eof())
            ss >> desc.geometry;
        PG_ASSERT(!in.fail() && !ss.fail());

        // vertex
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "fragment");
        if (!ss.eof())
            ss >> desc.fragment;
        PG_ASSERT(!in.fail() && !ss.fail());

        // compute
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "compute");
        if (!ss.eof())
            ss >> desc.compute;
        PG_ASSERT(!in.fail() && !ss.fail());

        addShaderRootDir(desc, PG_RESOURCE_DIR);

        ShaderTimeStamp newStamp = ShaderTimeStamp(desc);
        if (Resource::getShader(name)) {
            auto& currentStamp = shaderTimeStamps_[name];
            if (desc != currentStamp.desc || currentStamp.outOfDate(newStamp)) {
                LOG("Need to update shader: ", name);
                currentStamp = newStamp;

                Shader shader;
                if (!loadShaderFromText(shader, desc)) {
                    LOG("Could not load shader ", name);
                    return false;
                }
                updatedShaders_[name] = std::move(shader);
            } {
                LOG("already up to date shader: ", name);
            }
        } else {
            shaderTimeStamps_[name] = newStamp;
            if (!loadShaderFromText(shaders_[name], desc)) {
                LOG("Could not load shader ", name);
                shaders_.erase(name);
                return false;
            }
        }

        return true;
    }
} }
