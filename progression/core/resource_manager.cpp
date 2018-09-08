#include "core/resource_manager.h"
#include "graphics/mesh.h"
#include "tinyobjloader/tiny_obj_loader.h"

#include <functional>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

/*
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

    std::unordered_map<std::string, Model> ResourceManager::models_ = {};
    std::unordered_map<std::string, Material> ResourceManager::materials_ = {};
    std::unordered_map<std::string, Shader> ResourceManager::shaders_ = {};
    std::string ResourceManager::rootResourceDir_ = "";

    void ResourceManager::Init(const config::Config& config) {
        auto& rm = config["resourceManager"];
        if (rm) {
            if (rm["rootDirectory"])
                rootResourceDir_ = rm["rootDirectory"].as<std::string>();
        }

        // load defaults
        materials_["default"] = Material();
        shaders_["default-mesh"] = Shader(rootResourceDir_ + "shaders/regular_phong.vert", rootResourceDir_ + "shaders/regular_phong.frag");
        shaders_["default-mesh"].AddUniform("lights");
    }

    // TODO: implement
    void ResourceManager::Free() {
        
    }

    Model* ResourceManager::LoadModel(const std::string& filename) {
        std::string fullFileName = rootResourceDir_ + filename;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fullFileName.c_str(), rootResourceDir_.c_str(), true);

        if (!err.empty()) { // `err` may contain warning message.
            std::cerr << err << std::endl;
        }

        if (!ret) {
            std::cout << "Failed to load the input file: " << fullFileName << std::endl;
            return nullptr;
        }
        
        Model model;
 
        std::cout << "num materials: " << materials.size() << std::endl;
        for (int currentMaterialID = -1; currentMaterialID < (int) materials.size(); ++currentMaterialID) {
            Material* currentMaterial;
            if (currentMaterialID == -1) {
                currentMaterial = ResourceManager::GetMaterial("default");
            } else {
                tinyobj::material_t& mat = materials[currentMaterialID];
                glm::vec3 ambient(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
                glm::vec3 diffuse(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                glm::vec3 specular(mat.specular[0], mat.specular[1], mat.specular[2]);
                float shinyness = mat.shininess;
                currentMaterial = new Material(ambient, diffuse, specular, shinyness);
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

            if (verts.size()) {
                Mesh* currentMesh = new Mesh(verts.size(), verts.size() / 3, &verts[0], &normals[0], &uvs[0], &indices[0]);
                currentMesh->UploadToGPU(true, false);

                model.meshMaterialPairs.emplace_back(currentMesh, currentMaterial);
            }
        }

        models_[filename] = std::move(model);
        
        return &models_[filename];
    }

} // namespace Progression