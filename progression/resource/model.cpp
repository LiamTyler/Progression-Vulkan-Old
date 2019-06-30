#include "resource/model.hpp"
#include "resource/material.hpp"
#include "tinyobjloader/tiny_obj_loader.h"
#include "resource/resource_manager.hpp"
#include "core/common.hpp"
#include "utils/logger.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

class Vertex {
public:
    Vertex() :
        vertex(glm::vec3(0)),
        normal(glm::vec3(0)),
        uv(glm::vec3(0))
    {
    }

    Vertex(const glm::vec3& vert, const glm::vec3& norm, const glm::vec2& tex) :
        vertex(vert),
        normal(norm),
        uv(tex)
    {
    }

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

    void Model::optimize() {
        for (Mesh& mesh : meshes)
            mesh.optimize();
    }

    bool ModelMetaData::update() {
        return file.update();
    }

    Model::Model(const std::string& _name, const ModelMetaData& _metaData) :
        Resource(_name),
        metaData(_metaData)
    {
    }

    Model::Model(Model&& model) {
        *this = std::move(model);
    }

    Model& Model::operator=(Model&& model) {
        meshes = std::move(model.meshes);
        materials = std::move(model.materials);

        return *this;
    }

    void Model::move(Resource* resource) {
        Model* model = (Model*) resource;
        *model = std::move(*this);
    }

    Resource* Model::needsReloading() {
        if (metaData.update()) {
            return new Model(name, metaData);
        }
        return nullptr;
    }

    bool Model::load(MetaData* data) {
        if (data)
            metaData = *(ModelMetaData*) data;

        return loadFromObj();
    }

    bool Model::loadFromResourceFile(std::istream& in) {
        std::string line;
        std::string s;
        std::istringstream ss;

        // model name
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "name");
        ss >> name;
        PG_ASSERT(!in.fail() && !ss.fail());

        // model filename
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "filename");
        ss >> s;
        PG_ASSERT(!in.fail() && !ss.fail());
        metaData.file = TimeStampedFile(PG_RESOURCE_DIR + s);

        // optimize
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "optimize");
        ss >> s;
        PG_ASSERT(s == "true" || s == "false");
        metaData.optimize = s == "true";
        PG_ASSERT(!in.fail() && !ss.fail());

        // freeCpuCopy
        std::getline(in, line);
        ss = std::istringstream(line);
        ss >> s;
        PG_ASSERT(s == "freeCPUCopy");
        ss >> s;
        PG_ASSERT(s == "true" || s == "false");
        metaData.freeCpuCopy = s == "true";
        PG_ASSERT(!in.fail() && !ss.fail());

        return load();
    }

    // fname is full path
    bool Model::loadFromObj() {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> tiny_materials;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &tiny_materials, &err, metaData.file.filename.c_str(), PG_RESOURCE_DIR, true);

        if (!err.empty()) {
            LOG_WARN("TinyObj loader warning: ", err);
        }

        if (!ret) {
            LOG_ERR("Failed to load the obj file: ", metaData.file.filename);
            return false;
        }

        LOG("loading model = ", metaData.file.filename);
        LOG("loaded materials = ", tiny_materials.size());

        meshes.clear();
        materials.clear();

        for (int currentMaterialID = -1; currentMaterialID < (int) tiny_materials.size(); ++currentMaterialID) {
            Material* currentMaterial = new Material;
            if (currentMaterialID == -1) {
                currentMaterial->name = "default";
            } else {
                tinyobj::material_t& mat  = tiny_materials[currentMaterialID];
                currentMaterial->name = mat.name;
                currentMaterial->Ka = glm::vec3(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
                currentMaterial->Kd = glm::vec3(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
                currentMaterial->Ks = glm::vec3(mat.specular[0], mat.specular[1], mat.specular[2]);
                currentMaterial->Ke = glm::vec3(mat.emission[0], mat.emission[1], mat.emission[2]);
                currentMaterial->Ns = mat.shininess;
                currentMaterial->map_Kd_name = mat.diffuse_texname;
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
                if (metaData.optimize)
                    currentMesh.optimize();

                //currentMesh.uploadToGpu(metaData.freeCpuCopy);

                materials.push_back(currentMaterial);
                meshes.push_back(std::move(currentMesh));
            }
        }

        return true;
    }

} // namespace Progression
