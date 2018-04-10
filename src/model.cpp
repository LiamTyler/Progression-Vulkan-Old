#include "include/model.h"
#include <algorithm>

Model::Model() : Model("")
{
}

Model::Model(const std::string& fname) {
    filename_ = fname;
}

Model::~Model() {
    Free();
}

bool Model::Load() {
    objl::Loader Loader;
    bool loaded = Loader.LoadFile(filename_);
    if (!loaded) {
        std::cout << "Failed to open/load model: " << filename_ << std::endl;
        return false;
    }
    std::cout << "Loaded materials: " << Loader.LoadedMaterials.size() << std::endl;
    for (auto& m : Loader.LoadedMeshes) {
        bool use_mat = true;
        if (m.MeshMaterial.name == "" || m.MeshMaterial.name == "None")
            use_mat = false;

        Mesh* mesh = new Mesh;
        mesh->Load(m, use_mat);
        meshes_.push_back(mesh);
    }
    return true;
}

void Model::Free() {
    for (auto& mat : materials_) {
        delete mat;
    }
    for (auto& mesh : meshes_) {
        mesh->Free();
        delete mesh;
    }
}
