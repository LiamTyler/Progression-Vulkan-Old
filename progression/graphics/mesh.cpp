#include "graphics/mesh.h"
#include "tinyobjloader/tiny_obj_loader.h"

namespace Progression {

    Mesh::Mesh() :
        numVertices(0),
        numTriangles(0),
        vertices(nullptr),
        normals(nullptr),
        uvs(nullptr),
        indices(nullptr)
    {
        for (int i = 0; i < vboName::TOTAL_VBOS; ++i) vbos[i] = -1;
    }

    Mesh::Mesh(int numVerts, int numTris, glm::vec3* verts, glm::vec3* norms, glm::vec2* texCoords, unsigned int* _indices) :
        numVertices(numVerts),
        numTriangles(numTris),
        vertices(verts),
        normals(norms),
        uvs(texCoords),
        indices(_indices)
    {
        for (int i = 0; i < vboName::TOTAL_VBOS; ++i) vbos[i] = -1;
    }

    Mesh::~Mesh() {
        if (vbos[0] != -1)  {
            GLuint total = vboName::TOTAL_VBOS;
            if (vbos[vboName::UV] == -1)
                total -= 1;
            glDeleteBuffers(total, vbos);
        }
        if (vertices)
            delete[] vertices;
        if (normals)
            delete[] normals;
        if (uvs)
            delete[] uvs;
        if (indices)
            delete[] indices;
    }

    Mesh::Mesh(Mesh&& mesh) {
        *this = std::move(mesh);
    }

    Mesh& Mesh::operator=(Mesh&& mesh) {
        numVertices = mesh.numVertices;
        numTriangles = mesh.numTriangles;
        vertices = mesh.vertices;
        normals = mesh.normals;
        uvs = mesh.uvs;
        indices = mesh.indices;
        mesh.numVertices = 0;
        mesh.numTriangles = 0;
        mesh.vertices = nullptr;
        mesh.normals = nullptr;
        mesh.uvs = nullptr;
        mesh.indices = nullptr;

        for (int i = 0; i < vboName::TOTAL_VBOS; ++i) {
            vbos[i] = mesh.vbos[i];
            mesh.vbos[i] = -1;
        }

        return *this;
    }

    void Mesh::UploadToGPU(bool nullTheBuffers, bool freeMemory) {
        if (uvs)
            glGenBuffers(vboName::TOTAL_VBOS, vbos);
        else
            glGenBuffers(vboName::TOTAL_VBOS - 1, vbos);

        glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::VERTEX]);
        glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), vertices, GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::NORMAL]);
        glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec3), normals, GL_STATIC_DRAW);

        if (uvs) {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::UV]);
            glBufferData(GL_ARRAY_BUFFER, numVertices * sizeof(glm::vec2), uvs, GL_STATIC_DRAW);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[vboName::INDEX]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * numTriangles * sizeof(unsigned int), indices, GL_STATIC_DRAW);

        if (freeMemory) {
            delete[] vertices;
            delete[] normals;
            if (uvs)
                delete[] uvs;
            delete[] indices;
        }

        if (nullTheBuffers) {
            vertices = nullptr;
            normals = nullptr;
            uvs = nullptr;
            indices = nullptr;
        }
    }

} // namespace Progression