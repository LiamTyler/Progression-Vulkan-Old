#pragma once

#include "include/utils.h"
#include "include/OBJ_Loader.h"

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
