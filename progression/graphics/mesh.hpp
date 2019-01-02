#pragma once

#include "core/common.hpp"
#include "utils/noncopyable.hpp"
#include <vector>

namespace Progression {

    class Mesh : public NonCopyable {
    public:
        enum vboName : unsigned int {
            VERTEX,
            NORMAL,
            INDEX,
            UV,
            TOTAL_VBOS
        };

        Mesh();
        virtual ~Mesh();

        Mesh(Mesh&& mesh);
        Mesh& operator=(Mesh&& mesh);

		void UploadToGPU(bool freeCPUCopy = true);
		void Free(bool gpuCopy = true, bool cpuCopy = true);

        bool gpuCopyCreated() const { return vbos[vboName::VERTEX] != -1; }
        bool hasUVBuffer() const { return vbos[vboName::UV] != -1; }
        bool hasIndexBuffer() const { return vbos[vboName::INDEX] != -1; }
        unsigned int getNumVertices() const { return vertices.size() ? vertices.size() : numVertices_; }
        unsigned int getNumIndices() const { return indices.size() ? indices.size() : numIndices_; }


		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<glm::vec2> uvs;
		std::vector<unsigned int> indices;
        GLuint vbos[vboName::TOTAL_VBOS];
        bool dynamic = false;

	private:
        unsigned int numVertices_ = 0;
        unsigned int numIndices_ = 0;
		unsigned int maxVertices_ = 0;
		unsigned int maxIndices_ = 0;
    };

} // namespace Progression