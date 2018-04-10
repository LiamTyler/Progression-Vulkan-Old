#pragma once

#include "include/utils.h"
#include "include/OBJ_Loader.h"

class Mesh {
    public:
        Mesh();
        Mesh(const std::string& filename);

        ~Mesh();

        void Load(const std::string& filename);
        void Load(const objl::Mesh& mesh);
        void Free();

        unsigned int numVertices;
        unsigned int numTriangles;
        glm::vec3* vertices;
        glm::vec3* normals;
        glm::vec2* texCoords;
        glm::ivec3* indices;
};
