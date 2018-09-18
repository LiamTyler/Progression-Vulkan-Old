#include "core/resource_manager.h"
#include "graphics/mesh.h"
#include "tinyobjloader/tiny_obj_loader.h"

#include <functional>
#include <filesystem>
#include <fstream>

/*
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
namespace { namespace filescope {

    class Vertex {
    public:
        Vertex(const glm::vec3& vert, const glm::vec3& norm, const glm::vec2& tex) :
            vertex(vert),
            normal(norm),
            uv(tex) {}

        bool operator==(const Vertex& other) const {
            return vertex == other.vertex && normal == other.normal && uv == other.uv;
        }

        glm::vec3 vertex;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    namespace std {
    template<> struct hash<Vertex> {
    size_t operator()(Vertex const& vertex) const {
    return ((hash<glm::vec3>()(vertex.vertex) ^
    (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
    (hash<glm::vec2>()(vertex.uv) << 1);
    }
    };
    }         

} } // namespace anonymous::filescope
*/


namespace Progression {

    std::unordered_map<std::string, std::shared_ptr<Model>> ResourceManager::models_ = {};
    std::unordered_map<std::string, std::shared_ptr<Material>> ResourceManager::materials_ = {};
    std::unordered_map<std::string, std::shared_ptr<Shader>> ResourceManager::shaders_ = {};
    std::unordered_map<std::string, std::shared_ptr<Texture>> ResourceManager::textures_ = {};
    std::unordered_map<std::string, std::shared_ptr<Skybox>> ResourceManager::skyboxes_ = {};
    std::string ResourceManager::rootResourceDir_ = "";

    void ResourceManager::Init(const config::Config& config) {
        auto& rm = config["resourceManager"];
        if (rm) {
            if (rm["rootDirectory"])
                rootResourceDir_ = rm["rootDirectory"].as<std::string>();
        }

        // load defaults
        // shaders_["default-mesh"] = Shader(rootResourceDir_ + "shaders/regular_phong.vert", rootResourceDir_ + "shaders/regular_phong.frag");
        shaders_["default-mesh"] = std::make_shared<Shader>(rootResourceDir_ + "shaders/regular_phong.vert", rootResourceDir_ + "shaders/regular_phong.frag");
        shaders_["default-mesh"]->AddUniform("lights");
        // shaders_["skybox"] = Shader(rootResourceDir_ + "shaders/skybox.vert", rootResourceDir_ + "shaders/skybox.frag");
        shaders_["skybox"] = std::make_shared<Shader>(rootResourceDir_ + "shaders/skybox.vert", rootResourceDir_ + "shaders/skybox.frag");
        materials_["default"] = std::make_shared<Material>();
        materials_["default"]->shader = shaders_["default-mesh"].get();
        models_["plane"] = LoadModel("models/plane.obj");
    }

    // TODO: implement
    void ResourceManager::Free() {
        
    }

    void ResourceManager::LoadResourceFile(const std::string& relativePath) {
        std::filesystem::path path(rootResourceDir_ + relativePath);
        if (!std::filesystem::exists(path)) {
            std::cout << "File does not exist: " << path << std::endl;
            return;
        }
        
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) {
            if (line == "")
                continue;
            if (line[0] == '#')
                continue;
            if (line == "Skybox") {
                std::string name = "";
                line = " ";
                std::string right, left, top, bottom, front, back;
                while (line != "") {
                    std::getline(in, line);
                    std::stringstream ss(line);
                    std::string first;
                    ss >> first;
                    if (first == "name") {
                        ss >> name;
                    } else if (first == "right") {
                        ss >> right;
                    } else if (first == "left") {
                        ss >> left;
                    } else if (first == "top") {
                        ss >> top;
                    } else if (first == "bottom") {
                        ss >> bottom;
                    } else if (first == "front") {
                        ss >> front;
                    } else if (first == "back") {
                        ss >> back;
                    }
                }
                right = rootResourceDir_ + right;
                left = rootResourceDir_ + left;
                top = rootResourceDir_ + top;
                bottom = rootResourceDir_ + bottom;
                front = rootResourceDir_ + front;
                back = rootResourceDir_ + back;
                if (skyboxes_.find(name) != skyboxes_.end())
                    std::cout << "warning: resource manager already contains a skybox with the name '" << name << "'. Overriding with the new skybox" << std::endl;
                skyboxes_[name] = std::make_shared<Skybox>(right, left, top, bottom, front, back);
            } else if (line == "Material") {
                auto material = std::make_shared<Material>();
                std::string name = "";
                line = " ";
                while (line != "") {
                    std::getline(in, line);
                    std::stringstream ss(line);
                    std::string first;
                    ss >> first;
                    if (first == "name") {
                        ss >> name;
                    } else if (first == "ka") {
                        float x, y, z;
                        ss >> x >> y >> z;
                        material->ambient = glm::vec3(x, y, z);
                    } else if (first == "kd") {
                        float x, y, z;
                        ss >> x >> y >> z;
                        material->diffuse = glm::vec3(x, y, z);
                    } else if (first == "ks") {
                        float x, y, z;
                        ss >> x >> y >> z;
                        material->specular = glm::vec3(x, y, z);
                    } else if (first == "ns") {
                        float x;
                        ss >> x;
                        material->shininess = x;
                    } else if (first == "diffuseTex") {
                        std::string filename;
                        ss >> filename;
                        auto tex = ResourceManager::LoadTexture(filename);
                        if (!tex)
                            std::cout << "Warning: Material '" << name << "'s diffuse texture: " << filename << " not yet loaded. Setting to nullptr" << std::endl;
                        material->diffuseTexture = tex.get();
                    } else if (first == "shader") {
                        std::string tmp;
                        ss >> tmp;
                        auto shader = ResourceManager::GetShader(tmp);
                        if (!shader)
                            std::cout << "Warning: Material '" << name << "'s shader: " << tmp << " not yet loaded. Setting to nullptr" << std::endl;
                        material->shader = shader.get();
                    }
                }

                if (materials_.find(name) != materials_.end())
                    std::cout << "warning: resource manager already contains a material with the name '" << name << "'. Overriding with the new material" << std::endl;
                materials_[name] = material;
            } else if (line == "Model") {
                std::string filename, material, name;
                line = " ";
                while (line != "") {
                    std::getline(in, line);
                    std::stringstream ss(line);
                    std::string first;
                    ss >> first;
                    if (first == "filename") {
                        ss >> filename;
                    } else if (first == "material") {
                        ss >> material;
                    } else if (first == "name") {
                        ss >> name;
                    }
                }
                auto model = LoadModel(filename);
                if (name != "") {
                    model = std::make_shared<Model>(*model);
                    models_[name] = model;
                }
                if (material != "") {
                    if (materials_.find(material) == materials_.end()) {
                        std::cout << "warning: resource manager has no material with name '" << material << "'. Sticking with the model's default materials." << std::endl;
                        continue;
                    }
                    auto mat = GetMaterial(material, false);
                    for (int i = 0; i < model->materials.size(); ++i)
                        model->materials[i] = mat;
                }
            } else if (line == "Texture") {
                std::string filename;
                line = " ";
                while (line != "") {
                    std::getline(in, line);
                    std::stringstream ss(line);
                    std::string first;
                    ss >> first;
                    if (first == "filename") {
                        ss >> filename;
                    }
                }
                LoadTexture(filename);
            } else if (line == "Shader") {
                std::string name, vertex, frag;
                line = " ";
                while (line != "") {
                    std::getline(in, line);
                    std::stringstream ss(line);
                    std::string first;
                    ss >> first;
                    if (first == "name") {
                        ss >> name;
                    } else if (first == "vertex") {
                        ss >> vertex;
                    } else if (first == "fragment") {
                        ss >> frag;
                    }
                }
                if (shaders_.find(name) != shaders_.end()) {
                    std::cout << "warning: resource manager already contains a shader with name '" << name << "'. Skipping this one" << std::endl;
                    continue;
                }
                shaders_[name] = std::make_shared<Shader>(vertex, frag);
            }
        }

        in.close();
        
    }

    std::shared_ptr<Model> ResourceManager::LoadModel(const std::string& relativePath, bool addToManager) {
        if (models_.find(relativePath) != models_.end())
            return models_[relativePath];

        std::string fullFileName = rootResourceDir_ + relativePath;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fullFileName.c_str(), rootResourceDir_.c_str(), true);

        if (!err.empty()) {
            std::cerr << err << std::endl;
        }

        if (!ret) {
            std::cout << "Failed to load the input file: " << fullFileName << std::endl;
            return nullptr;
        }
        
        auto model = std::make_shared<Model>();
 
        for (int currentMaterialID = -1; currentMaterialID < (int) materials.size(); ++currentMaterialID) {
            std::shared_ptr<Material> currentMaterial;
            if (currentMaterialID == -1) {
                currentMaterial = ResourceManager::GetMaterial("default");
            } else {
                tinyobj::material_t& mat = materials[currentMaterialID];
                glm::vec3 ambient(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
                glm::vec3 diffuse(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                glm::vec3 specular(mat.specular[0], mat.specular[1], mat.specular[2]);
                float shinyness = mat.shininess;
                currentMaterial = std::make_shared<Material>(ambient, diffuse, specular, shinyness, nullptr, shaders_["default-mesh"].get());
            }

            std::vector<glm::vec3> verts;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> uvs;
            std::vector<unsigned int> indices;

            for (const auto& shape : shapes) {
                // Loop over faces(polygon)
                for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
                    if (shape.mesh.material_ids[f] == currentMaterialID) {
                        // Loop over vertices in the face. Each face should have 3 vertices from the LoadObj triangulation
                        for (size_t v = 0; v < 3; v++) {
                            tinyobj::index_t idx = shape.mesh.indices[3 * f + v];
                            tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                            tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                            tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                            verts.emplace_back(vx, vy, vz);

                            tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                            tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                            tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                            normals.emplace_back(nx, ny, nz);

                            if (idx.texcoord_index != -1) {
                                tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                                tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                                uvs.emplace_back(tx, ty);
                            }

                            indices.push_back(indices.size());
                        }
                    }
                }
            }

            //TODO: remove duplicate vertices from each mesh

            // create mesh and upload to GPU
            if (verts.size()) {
                auto currentMesh = std::make_shared<Mesh>(verts.size(), verts.size() / 3, &verts[0], &normals[0], &uvs[0], &indices[0]);
                currentMesh->UploadToGPU(true, false);

                model->meshes.push_back(currentMesh);
                model->materials.push_back(currentMaterial);
            }
        }

        if (addToManager)
            models_[relativePath] = model;
        
        return model;
    }

    std::shared_ptr<Skybox> ResourceManager::LoadSkybox(const std::string& name, const std::vector<std::string>& textures, bool addToManager) {
        if (skyboxes_.find(name) == skyboxes_.end()) {
            std::vector<std::string> fullTexturePaths; 
            for (const auto& tex : textures)
                fullTexturePaths.push_back(rootResourceDir_ + "skybox/" + tex);
            auto sp = std::make_shared<Skybox>(
                fullTexturePaths[0],
                fullTexturePaths[1],
                fullTexturePaths[2],
                fullTexturePaths[3],
                fullTexturePaths[4],
                fullTexturePaths[5]
            );
            if (addToManager)
                skyboxes_[name] = sp;
            else
                return sp;

        }
        return skyboxes_[name];
    }

    std::shared_ptr<Texture> ResourceManager::LoadTexture(const std::string& relativePath, bool addToManager) {
        if (textures_.find(relativePath) == textures_.end()) {
            auto sp = std::make_shared<Texture>(new Image(rootResourceDir_ + relativePath));
            if (addToManager)
                textures_[relativePath] = sp;
            else
                return sp;
        }
        return textures_[relativePath];
    }

    std::shared_ptr<Material> ResourceManager::AddMaterial(Material& material, const std::string& name) {
        if (materials_.find(name) != materials_.end())
            return nullptr;
        materials_[name] = std::make_shared<Material>(std::move(material));
        return materials_[name];
    }

    std::shared_ptr<Shader> ResourceManager::AddShader(Shader& shader, const std::string& name) {
        if (shaders_.find(name) != shaders_.end())
            return nullptr;
        shaders_[name] = std::make_shared<Shader>(std::move(shader));
        return shaders_[name];
    }

} // namespace Progression