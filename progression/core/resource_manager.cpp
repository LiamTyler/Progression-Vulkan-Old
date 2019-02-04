#include "core/resource_manager.hpp"
#include "graphics/mesh.hpp"
#include "tinyobjloader/tiny_obj_loader.h"
#include "core/time.hpp"
#include "core/common.hpp"
#include "utils/logger.hpp"
#include <functional>
#include <fstream>


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

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

namespace Progression {

    std::string ResourceManager::rootResourceDir = PG_RESOURCE_DIR;
    std::unordered_map<std::string, std::shared_ptr<Model>> ResourceManager::models_ = {};
    std::unordered_map<std::string, std::shared_ptr<Material>> ResourceManager::materials_ = {};
    std::unordered_map<std::string, std::shared_ptr<Shader>> ResourceManager::shaders_ = {};
    std::unordered_map<std::string, std::shared_ptr<Texture2D>> ResourceManager::textures2D_ = {};
    std::unordered_map<std::string, std::shared_ptr<Skybox>> ResourceManager::skyboxes_ = {};

    void ResourceManager::Init(const config::Config& config) {
        auto rm = config->get_table("resourceManager");
        // if (!rm) {
        // }

        // load defaults
        materials_["default"] = std::make_shared<Material>();
        models_["plane"] = LoadModel("models/plane.obj");
        models_["cube"] = LoadModel("models/cube.obj");
        models_["sphere"] = LoadModel("models/sphereHigh.obj");
        models_["sphereLow"] = LoadModel("models/sphereLow.obj");
        models_["cylinder"] = LoadModel("models/cylinder.obj");
    }

    // TODO: implement
    void ResourceManager::Free() {
        shaders_.clear();
        skyboxes_.clear();
        models_.clear();
        materials_.clear();
        textures2D_.clear();
    }

    bool ResourceManager::LoadResourceFile(const std::string& relativePath) {
        std::ifstream in(rootResourceDir + relativePath);
        if (!in) {
            LOG_ERR("Could not open resource file:", rootResourceDir + relativePath);
            return false;
        }

        std::string line;
        std::string tmpRootDir = "";
        while (std::getline(in, line)) {
            if (line == "")
                continue;
            if (line[0] == '#')
                continue;
            if (line == "RootResourceDir") {
                std::getline(in, line);
                std::stringstream ss(line);
                ss >> rootResourceDir;
            } else if (line == "Skybox") {
                std::string name = "";
                line = " ";
                std::string right, left, top, bottom, front, back;
                while (line != "" && !in.eof()) {
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

                right  = rootResourceDir + right;
                left   = rootResourceDir + left;
                front  = rootResourceDir + front;
                bottom = rootResourceDir + bottom;
                top    = rootResourceDir + top;
                back   = rootResourceDir + back;
                if (skyboxes_.find(name) != skyboxes_.end())
                    LOG_WARN("resource manager already contains a skybox with the name '", name, "'. Overriding with the new skybox");
                 auto sb = std::make_shared<Skybox>();
                 if (!sb->Load({ right, left, top, bottom, back, front })) {
                     LOG_ERR("Failed to load skybox");
                     continue;
                 }
                 skyboxes_[name] = sb;
            } else if (line == "Material") {
                auto material = std::make_shared<Material>();
                std::string name = "";
                line = " ";
                while (line != "" && !in.eof()) {
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
                    } else if (first == "ke") {
                        float x, y, z;
                        ss >> x >> y >> z;
                        material->emissive = glm::vec3(x, y, z);
                    } else if (first == "ns") {
                        float x;
                        ss >> x;
                        material->shininess = x;
                    } else if (first == "diffuseTex") {
                        std::string filename;
                        ss >> filename;
                        auto tex = ResourceManager::LoadTexture2D(filename);
                        if (!tex)
                            LOG_WARN("Warning: Material '", name, "'s diffuse texture : ", filename, " not yet loaded.Setting to nullptr");
                        material->diffuseTexture = tex;
                    }
                }

                if (materials_.find(name) != materials_.end())
                    LOG_WARN("resource manager already contains a material with the name '", name, "'. Overriding with the new material");
                materials_[name] = material;
            } else if (line == "Model") {
                std::string filename, material, name;
                line = " ";
                while (line != "" && !in.eof()) {
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
                    models_[name] = std::make_shared<Model>(*model);
                    model = models_[name];
                }
                if (material != "") {
                    if (materials_.find(material) == materials_.end()) {
                        LOG_WARN("warning: resource manager has no material with name '", material, "'. Sticking with the model's default materials.");
                        continue;
                    }
                    auto mat = GetMaterial(material);
                    for (size_t i = 0; i < model->materials.size(); ++i)
                        model->materials[i] = mat;
                }
            } else if (line == "Texture") {
                std::string filename;
                line = " ";
                while (line != "" && !in.eof()) {
                    std::getline(in, line);
                    std::stringstream ss(line);
                    std::string first;
                    ss >> first;
                    if (first == "filename") {
                        ss >> filename;
                    }
                }
                LoadTexture2D(filename);
            } else if (line == "Shader") {
                std::string name, vertex, frag;
                line = " ";
                while (line != "" && !in.eof()) {
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
                    LOG_WARN("resource manager already contains a shader with the name '", name, "'. Ignoring this one");
                    continue;
                }
                shaders_[name] = std::make_shared<Shader>();
                shaders_[name]->Load(vertex, frag);
            }
        }

        in.close();
        return true;
    }

    std::shared_ptr<Model> ResourceManager::LoadModel(const std::string& relativePath, bool addToManager) {
        if (models_.find(relativePath) != models_.end())
            return models_[relativePath];

        auto pos = relativePath.find_last_of('.');
        std::string ext = relativePath.substr(pos);
        std::shared_ptr<Model> model;
        if (ext == ".obj") {
            model = LoadOBJ(relativePath);
        } else if (ext == ".pgModel") {
            model = LoadPGModel(relativePath);
        } else {
            LOG_ERR("Trying to load model from unsupported file extension:", rootResourceDir + relativePath);
            return nullptr;
        }

        if (addToManager && model != nullptr)
            models_[relativePath] = model;

        return model;
    }

    std::shared_ptr<Model> ResourceManager::LoadPGModel(const std::string& relativePath) {
        std::string fullPath = rootResourceDir + relativePath;
        std::ifstream in(fullPath, std::ios::binary);
        if (!in) {
            LOG_ERR("Failed to load the pgModel file: ", fullPath);
            return nullptr;
        }

        auto model = std::make_shared<Model>();

        int numMeshes, numMaterials;
        in.read((char*)&numMeshes, sizeof(int));
        in.read((char*)&numMaterials, sizeof(int));
        model->meshes.resize(numMeshes);
        model->materials.resize(numMeshes);

        std::vector<std::shared_ptr<Material>> materials(numMaterials);
        std::vector<std::string> diffuseTexNames;
        std::string texName;
        // parse all of the materials
        for (int i = 0; i < numMaterials; ++i) {
            auto mat = std::make_shared<Material>();

            in.read((char*)&mat->ambient, sizeof(glm::vec3));
            in.read((char*)&mat->diffuse, sizeof(glm::vec3));
            in.read((char*)&mat->specular, sizeof(glm::vec3));
            in.read((char*)&mat->emissive, sizeof(glm::vec3));
            in.read((char*)&mat->shininess, sizeof(float));
            unsigned int texNameSize;
            in.read((char*)&texNameSize, sizeof(unsigned int));

            if (texNameSize != 0) {
                texName.resize(texNameSize);
                in.read(&texName[0], sizeof(char) * texNameSize);
                diffuseTexNames.push_back(texName);
            } else {
                diffuseTexNames.push_back("");
            }

            materials[i] = mat;
        }

        // parse all of the meshes
        for (int i = 0; i < numMeshes; ++i) {
            auto mesh = std::make_shared<Mesh>();
            unsigned int numVertices, numIndices, materialIndex;
            bool textured;

            in.read((char*)&numVertices, sizeof(unsigned int));
            in.read((char*)&numIndices, sizeof(unsigned int));
            in.read((char*)&materialIndex, sizeof(unsigned int));
            in.read((char*)&textured, sizeof(bool));

            mesh->vertices.resize(numVertices);
            mesh->normals.resize(numVertices);
            if (textured)
                mesh->uvs.resize(numVertices);
            if (numIndices)
                mesh->indices.resize(numIndices);

            // read in the mesh data
            in.read((char*)&mesh->vertices[0], numVertices * sizeof(glm::vec3));
            in.read((char*)&mesh->normals[0], numVertices * sizeof(glm::vec3));
            if (textured)
                in.read((char*)&mesh->uvs[0], numVertices * sizeof(glm::vec2));
            if (numIndices)
                in.read((char*)&mesh->indices[0], numIndices * sizeof(unsigned int));

            // create and upload the mesh
            mesh->UploadToGPU(true);

            model->meshes[i] = mesh;
            model->materials[i] = materials[materialIndex];
        }
        in.close();

        for (int i = 0; i < numMaterials; ++i) {
            if (diffuseTexNames[i] != "") {
                materials[i]->diffuseTexture = std::make_shared<Texture2D>();
                if (!materials[i]->diffuseTexture->Load(rootResourceDir + diffuseTexNames[i]))
                    materials[i]->diffuseTexture = nullptr;
            }
        }

        return model;
    }

    std::shared_ptr<Model> ResourceManager::LoadOBJ(const std::string& relativePath) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        std::string fullPath = rootResourceDir + relativePath;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fullPath.c_str(), rootResourceDir.c_str(), true);

        if (!err.empty()) {
            LOG_WARN("TinyObj loader warning: ", err);
        }

        if (!ret) {
            LOG_ERR("Failed to load the obj file: ", fullPath);
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
                glm::vec3 emissive(mat.emission[0], mat.emission[1], mat.emission[2]);
                float shininess = mat.shininess;
                std::shared_ptr<Texture2D> diffuseTex = nullptr;
                if (mat.diffuse_texname != "") {
                    diffuseTex = std::make_shared<Texture2D>();
                    if (!diffuseTex->Load(rootResourceDir + mat.diffuse_texname))
                        diffuseTex = nullptr;
                }

                currentMaterial = std::make_shared<Material>(ambient, diffuse, specular, emissive, shininess, diffuseTex);
            }

            auto mesh = std::make_shared<Mesh>();
            std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
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
                            //verts.emplace_back(vx, vy, vz);

                            tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                            tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                            tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                            //normals.emplace_back(nx, ny, nz);

                            tinyobj::real_t tx = 0, ty = 0;
                            if (idx.texcoord_index != -1) {
                                tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                                ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                                //uvs.emplace_back(tx, ty);
                            }


                            Vertex vertex(glm::vec3(vx, vy, vz), glm::vec3(nx, ny, nz), glm::vec2(ty, ty));
                            if (uniqueVertices.count(vertex) == 0) {
                                uniqueVertices[vertex] = static_cast<uint32_t>(mesh->vertices.size());
                                mesh->vertices.emplace_back(vx, vy, vz);
                                mesh->normals.emplace_back(nx, ny, nz);
                                if (idx.texcoord_index != -1)
                                    mesh->uvs.emplace_back(tx, ty);
                            }

                            mesh->indices.push_back(uniqueVertices[vertex]);
                        }
                    }
                }
            }

            // create mesh and upload to GPU
            if (mesh->vertices.size()) {
                mesh->UploadToGPU(true);

                model->meshes.push_back(mesh);
                model->materials.push_back(currentMaterial);
            }
        }

        return model;
    }

    bool ResourceManager::ConvertOBJToPGModel(const std::string& fullPathToOBJ, const std::string& fullPathToMaterialDir, const std::string& fullOutputPath) {
        std::ofstream outFile(fullOutputPath, std::ios::binary);
        if (!outFile) {
            LOG_ERR("Could not open output file:", fullOutputPath);
            return false;
        }
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fullPathToOBJ.c_str(), fullPathToMaterialDir.c_str(), true);

        if (!err.empty()) {
            LOG_WARN("Tinyobj warning:", err);
        }

        if (!ret) {
            LOG_ERR("Failed to load the input OBJ:", fullPathToOBJ);
            outFile.close();
            return false;
        }

        std::vector<int> materialList;
        std::vector<Mesh> meshList;
        for (int currentMaterialID = -1; currentMaterialID < (int) materials.size(); ++currentMaterialID) {
            std::vector<glm::vec3> verts;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> uvs;
            std::vector<unsigned int> indices;
            std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
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
                            //verts.emplace_back(vx, vy, vz);

                            tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                            tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                            tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                            //normals.emplace_back(nx, ny, nz);

                            tinyobj::real_t tx = 0, ty = 0;
                            if (idx.texcoord_index != -1) {
                                tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                                ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                                //uvs.emplace_back(tx, ty);
                            }

                            Vertex vertex(glm::vec3(vx, vy, vz), glm::vec3(nx, ny, nz), glm::vec2(ty, ty));
                            if (uniqueVertices.count(vertex) == 0) {
                                uniqueVertices[vertex] = static_cast<uint32_t>(verts.size());
                                verts.emplace_back(vx, vy, vz);
                                normals.emplace_back(nx, ny, nz);
                                if (idx.texcoord_index != -1)
                                    uvs.emplace_back(tx, ty);
                            }

                            indices.push_back(uniqueVertices[vertex]);
                        }
                    }
                }
            }

            // create mesh
            if (verts.size()) {
                Mesh m;

                m.vertices = std::move(verts);
                m.normals = std::move(normals);
                if (uvs.size()) {
                    m.uvs = std::move(uvs);
                }
                m.indices = std::move(indices);
                meshList.emplace_back(std::move(m));
                materialList.push_back(currentMaterialID);
            }
        }

        std::map<int, int> usedMaterialMap;
        for (const auto& matID : materialList) {
            if (!usedMaterialMap.count(matID))
                usedMaterialMap[matID] = usedMaterialMap.size();
        }

        unsigned int numMeshes = meshList.size();
        unsigned int numMaterials = usedMaterialMap.size();
        outFile.write((char*) &numMeshes, sizeof(unsigned int));
        outFile.write((char*) &numMaterials, sizeof(unsigned int));

        for (const auto& matID : usedMaterialMap) {
            glm::vec3 ambient, diffuse, specular, emissive;
            float shininess;
            unsigned int diffuseNameLength = 0;
            std::string diffuseTexName = "";

            if (matID.first == -1) {
                Material mat;
                ambient = mat.ambient;
                diffuse = mat.diffuse;
                specular = mat.specular;
                emissive = mat.emissive;
                shininess = mat.shininess;
            } else {
                tinyobj::material_t& mat = materials[matID.first];
                ambient = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
                diffuse = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                specular = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
                emissive = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
                shininess = mat.shininess;
                diffuseTexName = mat.diffuse_texname;
                diffuseNameLength = diffuseTexName.length();
            }
            outFile.write((char*) &ambient, sizeof(glm::vec3));
            outFile.write((char*) &diffuse, sizeof(glm::vec3));
            outFile.write((char*) &specular, sizeof(glm::vec3));
            outFile.write((char*) &emissive, sizeof(glm::vec3));
            outFile.write((char*) &shininess, sizeof(float));
            outFile.write((char*) &diffuseNameLength, sizeof(unsigned int));
            if (diffuseNameLength)
                outFile.write((char*) &diffuseTexName[0], sizeof(char) * diffuseNameLength);
        }

        for (size_t i = 0; i < meshList.size(); ++i) {
            const auto& mesh = meshList[i];
            unsigned int numVerts = mesh.vertices.size();
            unsigned int numIndices = mesh.indices.size();
            outFile.write((char*) &numVerts, sizeof(unsigned int));
            outFile.write((char*) &numIndices, sizeof(unsigned int));
            outFile.write((char*) &usedMaterialMap[materialList[i]], sizeof(unsigned int));
            bool textured = mesh.uvs.size() != 0;
            outFile.write((char*) &textured, sizeof(bool));
            outFile.write((char*) &mesh.vertices[0], sizeof(glm::vec3) * numVerts);
            outFile.write((char*) &mesh.normals[0], sizeof(glm::vec3) * numVerts);
            if (textured)
                outFile.write((char*) &mesh.uvs[0], sizeof(glm::vec2) * numVerts);
            outFile.write((char*) &mesh.indices[0], sizeof(unsigned int) * numIndices);
        }

        outFile.close();

        return true;
    }

    std::shared_ptr<Skybox> ResourceManager::LoadSkybox(const std::string& name, const std::vector<std::string>& textures, bool addToManager) {
        if (skyboxes_.find(name) == skyboxes_.end()) {
            std::vector<std::string> fullTexturePaths; 
            for (const auto& tex : textures)
                fullTexturePaths.push_back(rootResourceDir + tex);
            auto sp = std::make_shared<Skybox>();
            if (!sp->Load({
                    fullTexturePaths[0], // right
                    fullTexturePaths[1], // left
                    fullTexturePaths[2], // top
                    fullTexturePaths[3], // bottom
                    fullTexturePaths[4], // back
                    fullTexturePaths[5] // front
                }))
            {
                return nullptr;
            }

            if (addToManager)
                skyboxes_[name] = sp;
            else
                return sp;
        }

        return skyboxes_[name];
    }

    std::shared_ptr<Texture2D> ResourceManager::LoadTexture2D(const std::string& relativePath, bool addToManager) {
        if (textures2D_.find(relativePath) == textures2D_.end()) {
            auto sp = std::make_shared<Texture2D>();
            if (!sp->Load(rootResourceDir + relativePath))
                return nullptr;

            if (addToManager)
                textures2D_[relativePath] = sp;
            else
                return sp;
        }
        return textures2D_[relativePath];
    }

    std::shared_ptr<Material> ResourceManager::AddMaterial(Material& material, const std::string& name) {
        if (materials_.find(name) != materials_.end())
            return nullptr;
        materials_[name] = std::make_shared<Material>(std::move(material));
        return materials_[name];
    }

    std::shared_ptr<Shader> ResourceManager::AddShader(Shader&& shader, const std::string& name) {
        if (shaders_.find(name) != shaders_.end())
            return nullptr;
        shaders_[name] = std::make_shared<Shader>(std::move(shader));
        return shaders_[name];
    }

} // namespace Progression
