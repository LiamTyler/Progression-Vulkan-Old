#include <iostream>
#include "include/mesh.h"

Mesh::Mesh() {
    vertices = nullptr;
    normals = nullptr;
    indices = nullptr;
    numVertices = 0;
    numTriangles = 0;
}

Mesh::Mesh(const std::string& fname) {
    vertices = nullptr;
    normals = nullptr;
    indices = nullptr;
    numVertices = 0;
    numTriangles = 0;
    LoadMesh(fname);
}

Mesh::~Mesh() {
    if (vertices)
        delete vertices;
    if (normals)
        delete normals;
    if (indices)
        delete indices;
}

void Mesh::LoadMesh(const std::string& fname) {
    objl::Loader Loader;
    bool loaded = Loader.LoadFile(fname);
    if (!loaded) {
        std::cout << "Failed to open/load file: " << fname << std::endl;
        return;
    }
    objl::Mesh m = Loader.LoadedMeshes[0];
    numVertices = m.Vertices.size();
    vertices = new vec3[numVertices];
    normals  = new vec3[numVertices];
    for (int i = 0; i < numVertices; i++) {
        float x,y,z;
        x = m.Vertices[i].Position.X;
        y = m.Vertices[i].Position.Y;
        z = m.Vertices[i].Position.Z;
        vertices[i] = vec3(x, y, z);
        x = m.Vertices[i].Normal.X;
        y = m.Vertices[i].Normal.Y;
        z = m.Vertices[i].Normal.Z;
        normals[i] = vec3(x, y, z);
    }
    numTriangles = m.Indices.size() / 3;
    indices = new ivec3[numTriangles];
    int tri = 0;
    for (int i = 0; i < m.Indices.size();) {
        unsigned int x = m.Indices[i++];
        unsigned int y = m.Indices[i++];
        unsigned int z = m.Indices[i++];
        indices[tri++] = ivec3(x, y, z);
    }
}
