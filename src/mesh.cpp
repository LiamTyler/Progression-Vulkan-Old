#include "include/mesh.h"

Mesh::Mesh() {
    vertices = nullptr;
    normals = nullptr;
    normals = nullptr;
    texCoords = nullptr;
    indices = nullptr;
    numVertices = 0;
    numTriangles = 0;
}

Mesh::Mesh(const std::string& fname) {
    vertices = nullptr;
    normals = nullptr;
    texCoords = nullptr;
    indices = nullptr;
    numVertices = 0;
    numTriangles = 0;
    Load(fname);
}


Mesh::~Mesh() {
}

void Mesh::Free() {
    if (vertices)
        delete [] vertices;
    if (normals)
        delete [] normals;
    if (texCoords)
        delete [] texCoords;
    if (indices)
        delete [] indices;
}

void Mesh::Load(const std::string& fname) {
    objl::Loader Loader;
    bool loaded = Loader.LoadFile(fname);
    if (!loaded) {
        std::cout << "Failed to open/load file: " << fname << std::endl;
        return;
    }
    objl::Mesh m = Loader.LoadedMeshes[0];
    std::cout << "Loaded meshes size: " << Loader.LoadedMeshes.size() << std::endl;
    numVertices = m.Vertices.size();
    vertices = new glm::vec3[numVertices];
    normals  = new glm::vec3[numVertices];
    for (int i = 0; i < numVertices; i++) {
        float x,y,z;
        x = m.Vertices[i].Position.X;
        y = m.Vertices[i].Position.Y;
        z = m.Vertices[i].Position.Z;
        vertices[i] = glm::vec3(x, y, z);
        x = m.Vertices[i].Normal.X;
        y = m.Vertices[i].Normal.Y;
        z = m.Vertices[i].Normal.Z;
        normals[i] = glm::vec3(x, y, z);
    }
    numTriangles = m.Indices.size() / 3;
    indices = new glm::ivec3[numTriangles];
    int tri = 0;
    for (int i = 0; i < m.Indices.size();) {
        unsigned int x = m.Indices[i++];
        unsigned int y = m.Indices[i++];
        unsigned int z = m.Indices[i++];
        indices[tri++] = glm::ivec3(x, y, z);
    }
}

void Mesh::Load(const objl::Mesh& mesh) {
    numVertices = mesh.Vertices.size();
    vertices = new glm::vec3[numVertices];
    normals  = new glm::vec3[numVertices];
    for (int i = 0; i < numVertices; i++) {
        float x,y,z;
        x = mesh.Vertices[i].Position.X;
        y = mesh.Vertices[i].Position.Y;
        z = mesh.Vertices[i].Position.Z;
        vertices[i] = glm::vec3(x, y, z);
        x = mesh.Vertices[i].Normal.X;
        y = mesh.Vertices[i].Normal.Y;
        z = mesh.Vertices[i].Normal.Z;
        normals[i] = glm::vec3(x, y, z);
    }
    numTriangles = mesh.Indices.size() / 3;
    indices = new glm::ivec3[numTriangles];
    int tri = 0;
    for (int i = 0; i < mesh.Indices.size();) {
        unsigned int x = mesh.Indices[i++];
        unsigned int y = mesh.Indices[i++];
        unsigned int z = mesh.Indices[i++];
        indices[tri++] = glm::ivec3(x, y, z);
    }
}
