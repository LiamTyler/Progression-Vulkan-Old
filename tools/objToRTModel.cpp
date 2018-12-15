#include "progression.h"

// using namespace Progression;

#include <functional>
#include <fstream>
#include "tinyobjloader/tiny_obj_loader.h"
#include <algorithm>

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

/*
typedef struct Triangle {
    Triangle() : Triangle(0,0,0) {}

    Triangle(int a, int b, int c) : v1(a), v2(b), v3(c) {}

    glm::vec3 getCenter(glm::vec3* verts) const {
        return (verts[v1] + verts[v2] + verts[v3]) / 3.0f;
    }

    friend bool operator==(const Triangle& t1, const Triangle& t2) {
        return t1.v1 == t2.v1 && t1.v2 == t2.v2 && t1.v3 == t2.v3;
    }

    int v1, v2, v3;
} Triangle;
*/
typedef struct Triangle {
    Triangle() {}

    Triangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3,
             const glm::vec3& n1, const glm::vec3& n2, const glm::vec3& n3)
    {
        f1.x = v1.x; f1.y = v1.y; f1.z = v1.z;
        f1.w = v2.x; f2.x = v2.y; f2.y = v2.z;
        f2.z = v3.x; f2.w = v3.y; f3.x = v3.z;

        f3.y = n1.x; f3.z = n1.y; f3.w = n1.z;
        f4.x = n2.x; f4.y = n2.y; f4.z = n2.z;
        f4.w = n3.x; f5.x = n3.y; f5.y = n3.z;
    }

    glm::vec3 getCenter() const {
        glm::vec3 v1, v2, v3;
        v1 = glm::vec3(f1.x, f1.y, f1.z);
        v2 = glm::vec3(f1.w, f2.x, f2.y);
        v3 = glm::vec3(f2.z, f2.w, f3.x);
        return (v1 + v2 + v3) / 3.0f;
    }

    void getVerts(glm::vec3& v1, glm::vec3& v2, glm::vec3& v3) const {
        v1 = glm::vec3(f1.x, f1.y, f1.z);
        v2 = glm::vec3(f1.w, f2.x, f2.y);
        v3 = glm::vec3(f2.z, f2.w, f3.x);
    }

    glm::vec4 f1, f2, f3, f4, f5;
} Triangle;

typedef struct BoundingBox {
    BoundingBox() {}
    BoundingBox(const glm::vec3& _min, const glm::vec3& _max) : min(_min), max(_max) {}

    glm::vec3 min;
    glm::vec3 max;
} BoundingBox;

typedef struct RTMesh {
    RTMesh(const std::vector<Triangle>& tris,
            const std::vector<BoundingBox>& bbs,
            unsigned short m) :
        triangles(tris),
        boxes(bbs),
        matID(m)
    {
    }

    std::vector<Triangle> triangles;
    std::vector<BoundingBox> boxes;
    unsigned short matID;
} RTMesh;

class RBVH {
    public:
        RBVH() {
            left = nullptr;
            right = nullptr;
            min = glm::vec3(0);
            max = glm::vec3(0);
        }
        void Partition(
                const std::vector<BoundingBox>& boxes,
                const std::vector<Triangle>& triangles,
                const std::vector<int>& currShapes)
        {
            if (currShapes.size() == 0) {
                std::cout << "ERROR, TRYING TO PARTITION 0 SHAPES" << std::endl;
                return;
            }

            // find the bounding box for all shapes combined
            glm::vec3 curr_min, curr_max;
            float minX, maxX, minY, maxY, minZ, maxZ;
            // curr_min = boxes[0].min; curr_max = boxes[0].max;
            curr_min = boxes[currShapes[0]].min; curr_max = boxes[currShapes[0]].max;

            minX = curr_min.x; maxX = curr_max.x;
            minY = curr_min.y; maxY = curr_max.y;
            minZ = curr_min.z; maxZ = curr_max.z;
            glm::vec3 minC, maxC;
            minC = maxC = triangles[currShapes[0]].getCenter();

            for (int i = 1; i < currShapes.size(); ++i) {
                // get bounding box for shape
                // curr_min = boxes[i].min; curr_max = boxes[i].max;
                curr_min = boxes[currShapes[i]].min; curr_max = boxes[currShapes[i]].max;
                minX = std::min(minX, curr_min.x);
                minY = std::min(minY, curr_min.y);
                minZ = std::min(minZ, curr_min.z);
                maxX = std::max(maxX, curr_max.x);
                maxY = std::max(maxY, curr_max.y);
                maxZ = std::max(maxZ, curr_max.z);

                // find the min / max components of all of the shape centers
                // auto c = triangles[currShapes[i]].getCenter(verts);
                auto c = triangles[currShapes[i]].getCenter();
                minC.x = std::min(minC.x, c.x);
                minC.y = std::min(minC.y, c.y);
                minC.z = std::min(minC.z, c.z);
                maxC.x = std::max(maxC.x, c.x);
                maxC.y = std::max(maxC.y, c.y);
                maxC.z = std::max(maxC.z, c.z);

            }
            min = glm::vec3(minX, minY, minZ);
            max = glm::vec3(maxX, maxY, maxZ);

            glm::vec3 mid = (minC + maxC) / 2.0f;
            glm::vec3 d = maxC - minC;
            std::function<bool(const glm::vec3&, const glm::vec3&)> cmp;
            // split on largest axis
            if (d.x >= d.y && d.x >= d.z) {
                cmp = [&](glm::vec3 mid, glm::vec3 center) { return mid.x >= center.x; };
            } else if (d.y >= d.x && d.y >= d.z) {
                cmp = [&](glm::vec3 mid, glm::vec3 center) { return mid.y >= center.y; };
            } else {
                cmp = [&](glm::vec3 mid, glm::vec3 center) { return mid.z >= center.z; };
            }

            std::vector<int> left_tris;
            // std::vector<BoundingBox> left_boxes;
            std::vector<int> right_tris;
            // std::vector<BoundingBox> right_boxes;
            for (int i = 0; i < currShapes.size(); ++i) {
                if (cmp(mid, triangles[currShapes[i]].getCenter())) {
                    left_tris.push_back(currShapes[i]);
                    // left_boxes.push_back(boxes[i]);
                } else {
                    right_tris.push_back(currShapes[i]);
                    // right_boxes.push_back(boxes[i]);
                }
            }
            if (left_tris.size() == 0 || right_tris.size() == 0) {
                shapes_ = currShapes;
                if (left_tris.size() > 1 || right_tris.size() > 1)
                    std::cout << "num shapes in leaf: " << left_tris.size() << " " << right_tris.size() << std::endl;
            } else {
                left = new RBVH;
                right = new RBVH;
                // left->Partition(verts, left_boxes, triangles, left_tris);
                // right->Partition(verts, right_boxes, triangles, right_tris);
                left->Partition(boxes, triangles, left_tris);
                right->Partition(boxes, triangles, right_tris);
            }
        }

        int getNumShapes() { return shapes_.size(); }
        void getBB(glm::vec3& mi, glm::vec3& ma) { mi = min; ma = max; }
        int count() {
            int sum = 0;
            if (left)
                sum += left->count();
            if (right)
                sum += right->count();
            return 1 + sum;
        }

        RBVH* left;
        RBVH* right;
        std::vector<int> shapes_;
        glm::vec3 min;
        glm::vec3 max;
};

class IBVH {
    public:
        IBVH() {
            left = right = 0; // = numShapes = 0;
            min = max = glm::vec3(0);
        }
        IBVH(RBVH* node) {
            node->getBB(min, max);
            left = right = 0;
        }
        int getNumShapes() { left < 0 + right < 0; }
        bool isLeaf() { return left < 0; }
        void getBB(glm::vec3& mi, glm::vec3& ma) { mi = min; ma = max; }

        // int numShapes;
        glm::vec3 min;
        int left;
        glm::vec3 max;
        int right;
};

int insertChildren(IBVH* arr, RBVH* node, const std::vector<Triangle>& shapes, int parent, int size) {
    RBVH* l = node->left, *r = node->right;
    int numS = node->getNumShapes();

    if (numS) {
        if (numS > 2) {
            std::cout << "more than 2 shapes in this bin, error" << std::endl;
            // exit(EXIT_FAILURE);
        }

        arr[parent].left = -node->shapes_[0];
        if (numS >= 2) {
            arr[parent].right = -node->shapes_[1];
        }

        return size;
    }

    if (l) {
        arr[parent].left = size;
        arr[size++] = IBVH(l);
    }
    if (r) {
        arr[parent].right = size;
        arr[size++] = IBVH(r);
    }

    if (l)
        size = insertChildren(arr, l, shapes, arr[parent].left, size);
    if (r)
        size = insertChildren(arr, r, shapes, arr[parent].right, size);

    return size;
}

IBVH* CreateFromBVH(RBVH* root, const std::vector<Triangle>& shapes) {
    int nodes = root->count();
    std::cout << "nodes: " << nodes << std::endl;
    IBVH* list = new IBVH[nodes];
    list[0] = IBVH(root);
    int inserted = insertChildren(list, root, shapes, 0, 1);
    std::cout << "inserted nodes: " << inserted << std::endl;

    return list;
}

void writeToFile(
        const std::vector<std::pair<RTMesh, RTMaterial>>& list,
        const std::vector<IBVH*>& bvhs,
        const std::vector<int>& bvh_sizes,
        const std::string& filename)
{
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cout << "Could not open output file: " << filename << std::endl;
        exit(EXIT_FAILURE);
    }

    unsigned int numMeshes = list.size();
    outFile.write((char*) &numMeshes, sizeof(unsigned int));

    // first write the materials
    for (const auto& pair: list) {
        std::cout << "writing mat" << std::endl;
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
        unsigned int numTris  = mesh.triangles.size();
        // unsigned int numVerts = mesh.vertices.size();
        // outFile.write((char*) &numVerts, sizeof(unsigned int));
        outFile.write((char*) &numTris, sizeof(unsigned int));
        outFile.write((char*) &bvh_sizes[i], sizeof(int));
        // outFile.write((char*) &mesh.vertices[0], sizeof(glm::vec3) * numVerts);
        // outFile.write((char*) &mesh.normals[0], sizeof(glm::vec3) * numVerts);
        outFile.write((char*) &mesh.triangles[0], sizeof(Triangle) * numTris);
        outFile.write((char*) bvhs[i], sizeof(IBVH) * bvh_sizes[i]);
    }

    outFile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cout << "Usage: pathToOBJFile  pathToMaterialDirectory   pathToPGModelOutput" << std::endl;
        return 0;
    }
    std::cout << "sizeof IBVH = " << sizeof(IBVH) << std::endl;

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
    std::cout << "parsing" << std::endl;
    std::cout << "shapes.size() = " << shapes.size() << std::endl;
    std::cout << "materials.size() = " << materials.size() << std::endl;

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
        std::vector<BoundingBox> boxes;
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
                        verts.emplace_back(vx, vy, vz);

                        tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                        tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                        tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                        normals.emplace_back(nx, ny, nz);

                        tinyobj::real_t tx = 0, ty = 0;
                        if (idx.texcoord_index != -1) {
                            tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                            ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                            //uvs.emplace_back(tx, ty);
                        }


                        /*
                        Vertex vertex(glm::vec3(vx, vy, vz), glm::vec3(nx, ny, nz), glm::vec2(ty, ty));
                        if (uniqueVertices.count(vertex) == 0) {
                            uniqueVertices[vertex] = static_cast<uint32_t>(verts.size());
                            verts.emplace_back(vx, vy, vz);
                            normals.emplace_back(nx, ny, nz);
                            if (idx.texcoord_index != -1)
                                uvs.emplace_back(tx, ty);
                        }


                        indices.push_back(uniqueVertices[vertex]);                            
                        */
                    }
                    // tris.emplace_back(indices[indices.size() - 1], indices[indices.size() - 2], indices[indices.size() - 3]);
                    int i = verts.size() - 3;
                    tris.emplace_back(verts[i], verts[i+1], verts[i+2], normals[i], normals[i+1], normals[i+2]);

                    // calculate BB for triangle
                    const auto& tri = tris[tris.size() - 1];
                    // const glm::vec3 v1 = verts[tri.v1], v2 = verts[tri.v2], v3 = verts[tri.v3];
                    glm::vec3 v1, v2, v3;
                    tri.getVerts(v1, v2, v3);
                    glm::vec3 bbmin, bbmax;
                    bbmin.x = std::min(v1.x, std::min(v2.x, v3.x));
                    bbmin.y = std::min(v1.y, std::min(v2.y, v3.y));
                    bbmin.z = std::min(v1.z, std::min(v2.z, v3.z));
                    bbmax.x = std::max(v1.x, std::max(v2.x, v3.x));
                    bbmax.y = std::max(v1.y, std::max(v2.y, v3.y));
                    bbmax.z = std::max(v1.z, std::max(v2.z, v3.z));
                    boxes.emplace_back(bbmin, bbmax);
                }
            }
        }

        if (verts.size()) {
            // RTMesh currentMesh(verts, normals, tris, boxes, 0);
            RTMesh currentMesh(tris, boxes, 0);
            retList.emplace_back(currentMesh, currentMat);
        }
    }

    if (retList.size() > 1) {
        std::cout << "more than 1 mesh for this model, invalid" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::cout << "Done parsing, creating BVH" << std::endl;

    // calculate the BVHs for the model
    std::vector<IBVH*> bvhs;
    std::vector<int> bvh_sizes;
    std::cout << "num meshes: " << retList.size() << std::endl;
    for (const auto& pair : retList) {
        auto mesh = pair.first;
        // std::cout << "num verts = " << mesh.vertices.size() << std::endl;
        std::cout << "num tris  = " << mesh.triangles.size() << std::endl;
        RBVH rbvh;
        std::vector<int> shapeIdxs;
        shapeIdxs.resize(mesh.triangles.size());
        for (int i = 0; i < shapeIdxs.size(); ++i)
            shapeIdxs[i] = i;

        std::cout << "creating RBVH" << std::endl;
        // rbvh.Partition(&mesh.vertices[0], mesh.boxes, mesh.triangles, shapeIdxs);
        rbvh.Partition(mesh.boxes, mesh.triangles, shapeIdxs);
        std::cout << "bvh count: " << rbvh.count() << std::endl;
        IBVH* ibvh = CreateFromBVH(&rbvh, mesh.triangles);
        bvhs.push_back(ibvh);
        bvh_sizes.push_back(rbvh.count());
    }

    std::cout << "writing to file" << std::endl;
    writeToFile(retList, bvhs, bvh_sizes, rtModelPath);

    return 0;
}
