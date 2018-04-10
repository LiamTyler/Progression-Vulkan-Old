#pragma once

#include "include/utils.h"
#include "include/OBJ_Loader.h"
#include "include/material.h"

class Mesh {
    public:
        Mesh();
        Mesh(const std::string& filename);

        ~Mesh();

        void Free();
        void Load(const std::string& filename);
        // loading from Model class
        void Load(const objl::Mesh& mesh, bool use_mat);
        bool HasTextureCoords() { return texCoords != nullptr; }

        Material* material;
        unsigned int numVertices;
        unsigned int numTriangles;
        glm::vec3* vertices;
        glm::vec3* normals;
        glm::vec2* texCoords;
        glm::ivec3* indices;
};
