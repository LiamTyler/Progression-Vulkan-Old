#pragma once

#include <vector>

#include "include/mesh.h"
#include "include/material.h"

namespace Progression {

	class Model {
	public:
		Model(const std::string& fname = "");
		~Model();

		bool Load();
		void Free();

		std::string GetName() const { return filename_; }
		std::vector<Mesh*> GetMeshes() const { return meshes_; }
		std::vector<Material*> GetMaterials() const { return materials_; }

	protected:
		std::string filename_;
		std::vector<Mesh*> meshes_;
		std::vector<Material*> materials_;
	};

} // namespace Progression