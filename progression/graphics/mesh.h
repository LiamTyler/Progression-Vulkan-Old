#pragma once

#include "core/common.h"

namespace Progression {

    /**
    * TODO:
    *   - handle updating buffers
    *   - handle copying
    */
    class Mesh {
    public:
        enum vboName : unsigned int {
            VERTEX,
            NORMAL,
            INDEX,
            UV,
            TOTAL_VBOS
        };


        Mesh();
        Mesh(int numVerts, int numTris, glm::vec3* verts, glm::vec3* norms, glm::vec2* uvs, unsigned int* indices);
        virtual ~Mesh();

        Mesh(const Mesh& mesh) = delete;
        Mesh& operator=(const Mesh& mesh) = delete;
        Mesh(Mesh&& mesh);
        Mesh& operator=(Mesh&& mesh);

        void UploadToGPU(bool nullTheBuffers = true, bool freeMemory = true);

        unsigned int numVertices;
        unsigned int numTriangles;
        glm::vec3* vertices;
        glm::vec3* normals;
        glm::vec2* uvs;
        unsigned int* indices;

        GLuint vbos[vboName::TOTAL_VBOS];
    };

} // namespace Progression