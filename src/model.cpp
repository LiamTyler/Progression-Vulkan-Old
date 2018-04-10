#include "include/model.h"

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
    for (auto& m : Loader.LoadedMeshes) {
        Mesh* mesh = new Mesh;
        mesh->Load(m);
        meshes_.push_back(mesh);
    }
    return true;
}

void Model::Free() {
    for (auto& mesh : meshes_) {
        mesh->Free();
        delete mesh;
    }
}
