#include "progression.h"

using namespace Progression;

#include <functional>
#include <fstream>
#include "tinyobjloader/tiny_obj_loader.h"

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

typedef struct RTMaterial {
    RTMaterial() : RTMaterial(glm::vec3(0), glm::vec3(0), glm::vec3(0), 0, 0) {}

    RTMaterial(const glm::vec3& d, const glm::vec3& s, const glm::vec3& t, float spec, float i) :
        diffuse(d), specular(s), transmissive(t), shininess(spec), ior(i)
    {
    }

    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 transmissive;
    float shininess;
    float ior;
} RTMaterial;

typedef struct Triangle {
    Triangle() : Triangle(0,0,0) {}

    Triangle(int a, int b, int c) : v1(a), v2(b), v3(c) {}

    int v1, v2, v3;
} Triangle;

typedef struct RTMesh {
    RTMesh(const std::vector<glm::vec3>& verts,
            const std::vector<glm::vec3>& norms,
            const std::vector<Triangle>& tris,
            unsigned short m) :
        vertices(verts),
        normals(norms),
        triangles(tris),
        matID(m)
    {
    }

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<Triangle> triangles;
    unsigned short matID;
} RTMesh;

void writeToFile(const std::vector<std::pair<RTMesh, RTMaterial>>& list, const std::string& filename);

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage: pathToOBJFile  pathToMaterialDirectory   pathToPGModelOutput" << std::endl;
        return 0;
    }

    std::string fullPathToOBJ = argv[1];
    std::string materialDir   = argv[2];
    std::string rtModelPath   = argv[3];

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, fullPathToOBJ.c_str(), materialDir.c_str(), true);

    if (!err.empty()) {
        std::cerr << err << std::endl;
    }

    if (!ret) {
        std::cout << "Failed to load the input file: " << fullPathToOBJ << std::endl;
        return 0;
    }

    std::vector<std::pair<RTMesh, RTMaterial>> retList;
    for (int currentMaterialID = -1; currentMaterialID < (int) materials.size(); ++currentMaterialID) {
        RTMaterial currentMat;
        if (currentMaterialID != -1) {
            tinyobj::material_t& mat = materials[currentMaterialID];
            glm::vec3 diffuse(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
            glm::vec3 specular(mat.specular[0], mat.specular[1], mat.specular[2]);
            glm::vec3 transmissive(mat.transmittance[0], mat.transmittance[1], mat.transmittance[2]);
            float shininess = mat.shininess;
            float ior = mat.ior;

            currentMat = RTMaterial(diffuse, specular, transmissive, shininess, ior);
        }

        std::vector<glm::vec3> verts;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> uvs;
        std::vector<unsigned int> indices;
        std::vector<Triangle> tris;
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
                    tris.emplace_back(indices[indices.size() - 1], indices[indices.size() - 2], indices[indices.size() - 3]);
                }
            }
        }

        if (verts.size()) {
            RTMesh currentMesh(verts, normals, tris, 0);
            retList.emplace_back(currentMesh, currentMat);
        }
    }

    writeToFile(retList, rtModelPath);

    return 0;
}

void writeToFile(const std::vector<std::pair<RTMesh, RTMaterial>>& list, const std::string& filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cout << "Could not open output file: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    unsigned int numMeshes = list.size();
    outFile.write((char*) &numMeshes, sizeof(unsigned int));

    // first write the materials
    for (const auto& pair: list) {
        const auto& mat = pair.second;

        outFile.write((char*) &mat.diffuse, sizeof(glm::vec3));
        outFile.write((char*) &mat.specular, sizeof(glm::vec3));
        outFile.write((char*) &mat.transmissive, sizeof(glm::vec3));
        outFile.write((char*) &mat.shininess, sizeof(float));
        outFile.write((char*) &mat.ior, sizeof(float));
    }

    // now write meshes
    for (int i = 0; i < list.size(); ++i) {
        const auto& mesh = list[i].first;
        unsigned int numVerts = mesh.vertices.size();
        unsigned int numTris  = mesh.triangles.size();
        outFile.write((char*) &numVerts, sizeof(unsigned int));
        outFile.write((char*) &numTris, sizeof(unsigned int));
        outFile.write((char*) &mesh.vertices[0], sizeof(glm::vec3) * numVerts);
        outFile.write((char*) &mesh.normals[0], sizeof(glm::vec3) * numVerts);
        outFile.write((char*) &mesh.triangles[0], sizeof(Triangle) * numTris);
    }

    outFile.close();
}

