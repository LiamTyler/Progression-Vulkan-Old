#include "graphics/mesh.h"
#include "tinyobjloader/tiny_obj_loader.h"

namespace Progression {

    Mesh::Mesh() :
        numVertices_(0),
        numTriangles_(0),
        vertices_(nullptr),
        normals_(nullptr),
        uvs_(nullptr),
        indices_(nullptr)
    {
        glGenBuffers(vboName::TOTAL_VBOS, vbos_);
    }

    Mesh::Mesh(int numVerts, int numTris, glm::vec3* verts, glm::vec3* norms, glm::vec2* uvs, unsigned int* indices) :
        numVertices_(numVerts),
        numTriangles_(numTris),
        vertices_(verts),
        normals_(norms),
        uvs_(uvs),
        indices_(indices)
    {
        glGenBuffers(vboName::TOTAL_VBOS, vbos_);
    }

    Mesh::~Mesh() {
        if (vbos_[0] != -1)
            glDeleteBuffers(vboName::TOTAL_VBOS, vbos_);
        if (vertices_)
            delete[] vertices_;
        if (normals_)
            delete[] normals_;
        if (uvs_)
            delete[] uvs_;
        if (indices_)
            delete[] indices_;
    }

    Mesh::Mesh(Mesh&& mesh) {
        *this = std::move(mesh);
    }

    Mesh& Mesh::operator=(Mesh&& mesh) {
        numVertices_ = mesh.numVertices_;
        numTriangles_ = mesh.numTriangles_;
        vertices_ = mesh.vertices_;
        normals_ = mesh.normals_;
        uvs_ = mesh.uvs_;
        indices_ = mesh.indices_;
        mesh.numVertices_ = 0;
        mesh.numTriangles_ = 0;
        mesh.vertices_ = nullptr;
        mesh.normals_ = nullptr;
        mesh.uvs_ = nullptr;
        mesh.indices_ = nullptr;

        for (int i = 0; i < vboName::TOTAL_VBOS; ++i) {
            vbos_[i] = mesh.vbos_[i];
            mesh.vbos_[i] = -1;
        }

        return *this;
    }

    void Mesh::UploadToGPU(bool nullTheBuffers, bool freeMemory) {
        glBindBuffer(GL_ARRAY_BUFFER, vbos_[vboName::VERTEX]);
        glBufferData(GL_ARRAY_BUFFER, numVertices_ * sizeof(glm::vec3), vertices_, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, vbos_[vboName::NORMAL]);
        glBufferData(GL_ARRAY_BUFFER, numVertices_ * sizeof(glm::vec3), normals_, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, vbos_[vboName::UV]);
        glBufferData(GL_ARRAY_BUFFER, numVertices_ * sizeof(glm::vec2), uvs_, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[vboName::INDEX]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * numTriangles_ * sizeof(unsigned int), indices_, GL_STATIC_DRAW);

        if (freeMemory) {
            delete[] vertices_;
            delete[] normals_;
            delete[] uvs_;
            delete[] indices_;
        }

        if (nullTheBuffers) {
            vertices_ = nullptr;
            normals_ = nullptr;
            uvs_ = nullptr;
            indices_ = nullptr;
        }
    }

} // namespace Progression