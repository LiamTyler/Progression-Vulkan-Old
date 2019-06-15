#include "resource/resourceIO/model_io.hpp"
#include "tinyobjloader/tiny_obj_loader.h"
#include "resource/material.hpp"
#include "resource/mesh.hpp"
#include "resource/texture2D.hpp"
#include "resource/resourceIO/texture_io.hpp"
#include "utils/logger.hpp"

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

    // fname is full path
    bool loadModelFromObj(Model& model, const std::string& fname, bool freeCpuCopy) {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fname.c_str(), PG_RESOURCE_DIR, true);

        if (!err.empty()) {
            LOG_WARN("TinyObj loader warning: ", err);
        }

        if (!ret) {
            LOG_ERR("Failed to load the obj file: ", fname);
            return false;
        }

        for (int currentMaterialID = -1; currentMaterialID < (int) materials.size(); ++currentMaterialID) {
            LOG("Material id: ", currentMaterialID);
            Material currentMaterial;
            if (currentMaterialID == -1) {
                // default material
            } else {
                tinyobj::material_t& mat  = materials[currentMaterialID];
                currentMaterial.ambient   = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
                currentMaterial.diffuse   = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                currentMaterial.specular  = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
                currentMaterial.emissive  = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
                currentMaterial.shininess = mat.shininess;
                if (mat.diffuse_texname != "") {
                    currentMaterial.diffuseTexture = new Texture2D;
                    std::string texName = mat.diffuse_texname;
                    // TODO: if texture is already loaded, then use that one (able to specify usage desc)
                    // if not then just use the default texture usage
                    TextureUsageDesc texUsage;
                    if (!loadTexture2D(*currentMaterial.diffuseTexture, PG_RESOURCE_DIR + mat.diffuse_texname, texUsage, freeCpuCopy)) {
                        LOG_ERR("Unable to load material's texture: ", PG_RESOURCE_DIR + mat.diffuse_texname);
                        delete currentMaterial.diffuseTexture;
                        for (auto& m : model.materials)
                            if (m->diffuseTexture)
                                delete m->diffuseTexture;
                        return false;
                    }
                }
            }

            Mesh currentMesh;
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

                            tinyobj::real_t nx = 0;
                            tinyobj::real_t ny = 0;
                            tinyobj::real_t nz = 0;
                            if (idx.normal_index != -1) {
                                nx = attrib.normals[3 * idx.normal_index + 0];
                                ny = attrib.normals[3 * idx.normal_index + 1];
                                nz = attrib.normals[3 * idx.normal_index + 2];
                            }

                            tinyobj::real_t tx = 0, ty = 0;
                            if (idx.texcoord_index != -1) {
                                tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                                ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                            }

                            Vertex vertex(glm::vec3(vx, vy, vz), glm::vec3(nx, ny, nz), glm::vec2(ty, ty));
                            if (uniqueVertices.count(vertex) == 0) {
                                uniqueVertices[vertex] = static_cast<uint32_t>(currentMesh.vertices.size());
                                currentMesh.vertices.emplace_back(vx, vy, vz);
                                if (idx.normal_index != -1)
                                    currentMesh.normals.emplace_back(nx, ny, nz);
                                if (idx.texcoord_index != -1)
                                    currentMesh.uvs.emplace_back(tx, ty);
                            }

                            currentMesh.indices.push_back(uniqueVertices[vertex]);
                        }
                    }
                }
            }

            // create mesh and upload to GPU
            if (currentMesh.vertices.size()) {
                LOG("push back on mat id: ", currentMaterialID);
                currentMesh.uploadToGpu(true);

                model.materials.push_back(new Material(currentMaterial));
                model.meshes.push_back(std::move(currentMesh));
            }
        }

        return true;
    }

} // namespace Progression
