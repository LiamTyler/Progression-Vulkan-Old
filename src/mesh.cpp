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
    objl::Mesh mesh = Loader.LoadedMeshes[0];
    bool use_mat = true;
    if (mesh.MeshMaterial.name == "" || mesh.MeshMaterial.name == "None")
        use_mat = false;
    Load(mesh, use_mat);
}

void Mesh::Load(const objl::Mesh& mesh, bool use_mat) {
    numVertices = mesh.Vertices.size();
    vertices = new glm::vec3[numVertices];
    normals  = new glm::vec3[numVertices];
    // see if the mesh actually has tex coords
    bool hasTexCoords = false;
    for (int i = 0; i < numVertices && !hasTexCoords; i++)  {
        float x = mesh.Vertices[i].TextureCoordinate.X;
        float y = mesh.Vertices[i].TextureCoordinate.Y;
        if (x != 0 || y != 0)
            hasTexCoords = true;
    }
    if (hasTexCoords)
        texCoords = new glm::vec2[numVertices];
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
        if (hasTexCoords) {
            x = mesh.Vertices[i].TextureCoordinate.X;
            y = mesh.Vertices[i].TextureCoordinate.Y;
            texCoords[i] = glm::vec2(x, y);
        }
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
    if (use_mat) {
        objl::Material mat = mesh.MeshMaterial;
        glm::vec3 ka(mat.Ka.X, mat.Ka.Y, mat.Ka.Z);
        glm::vec3 kd(mat.Kd.X, mat.Kd.Y, mat.Kd.Z);
        glm::vec3 ks(mat.Ks.X, mat.Ks.Y, mat.Ks.Z);
        float spec = mat.Ns;
        material = new Material(ka, kd, ks, spec);

        std::string diffuse_map = mat.map_Kd;
        if (diffuse_map != "") {
            material->diffuseTex = new Texture(diffuse_map);
        } else {
            material->diffuseTex = nullptr;
        }
    } else {
        material = new Material;
    }
}
