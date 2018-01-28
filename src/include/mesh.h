#ifndef SRC_INCLUDE_MESH_H_
#define SRC_INCLUDE_MESH_H_

#include "include/OBJ_Loader.h"
#include "glm/glm.hpp"

using glm::vec3;
using glm::ivec3;

class Mesh {
    public:
        Mesh();
        Mesh(const std::string& filename);
        ~Mesh();

        void LoadMesh(const std::string& filename);

        unsigned int numVertices;
        unsigned int numTriangles;
        glm::vec3* vertices;
        glm::vec3* normals;
        glm::ivec3* indices;
};

#endif  // SRC_INCLUDE_MESH_H_
