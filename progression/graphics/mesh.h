#pragma once

#include "core/common.h"

namespace Progression {

    /**
    * TODO:
    *   - handle updating buffers
    *   - moving / copying
    */
    class Mesh {

        enum vboName : unsigned int {
            VERTEX,
            NORMAL,
            UV,
            INDEX,
            TOTAL_VBOS
        };

    public:
        Mesh();
        Mesh(int numVerts, int numTris, glm::vec3* verts, glm::vec3* norms, glm::vec2* uvs, unsigned int* indices);
        virtual ~Mesh();

        Mesh(const Mesh& mesh) = delete;
        Mesh& operator=(const Mesh& mesh) = delete;
        Mesh(const Mesh&& mesh) = delete;
        Mesh& operator=(const Mesh&& mesh) = delete;

        void UploadToGPU(bool nullTheBuffers = true, bool freeMemory = true);

        unsigned int getNumVertices() const { return numVertices_; }
        unsigned int getNumTriangles() const { return numTriangles_; }
        glm::vec3* getVertices() const { return vertices_; }
        glm::vec3* getNormals() const { return normals_; }
        glm::vec2* getUVs() const { return uvs_; }
        unsigned int* getIndices() const { return indices_; }
        GLuint* getBuffers() { return &vbos_[0]; }

    protected:
        unsigned int numVertices_;
        unsigned int numTriangles_;
        glm::vec3* vertices_;
        glm::vec3* normals_;
        glm::vec2* uvs_;
        unsigned int* indices_;

        GLuint vbos_[vboName::TOTAL_VBOS];
    };

} // namespace Progression