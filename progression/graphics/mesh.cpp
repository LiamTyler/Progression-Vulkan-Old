#include "graphics/mesh.hpp"

namespace Progression {

    Mesh::Mesh() {
        for (int i = 0; i < vboName::TOTAL_VBOS; ++i)
            vbos[i] = -1;
    }

    Mesh::~Mesh() {
        if (gpuCopyCreated()) {
            GLuint numBuffers = 2;
            if (hasUVBuffer())
                ++numBuffers;
            if (hasIndexBuffer())
                ++numBuffers;
            glDeleteBuffers(numBuffers, vbos);
        }
    }

    Mesh::Mesh(Mesh&& mesh) {
        *this = std::move(mesh);
    }

    Mesh& Mesh::operator=(Mesh&& mesh) {
        vertices = std::move(mesh.vertices);
        normals = std::move(mesh.normals);
        uvs = std::move(mesh.uvs);
        indices = std::move(mesh.indices);
        dynamic = std::move(mesh.dynamic);
        numVertices_ = std::move(mesh.numVertices_);
        numIndices_ = std::move(mesh.numIndices_);
        maxVertices_ = std::move(mesh.maxVertices_);
        maxIndices_ = std::move(mesh.maxIndices_);

        for (int i = 0; i < vboName::TOTAL_VBOS; ++i) {
            vbos[i] = mesh.vbos[i];
            mesh.vbos[i] = -1;
        }

        return *this;
    }

    void Mesh::UploadToGPU(bool freeCPUCopy) {
        if (!gpuCopyCreated()) {
            GLuint numBuffers = 2;
            if (uvs.size())
                ++numBuffers;
            else
                vbos[vboName::UV] = -1;

            if (indices.size())
                ++numBuffers;
            else
                vbos[vboName::INDEX] = -1;

            glGenBuffers(numBuffers, vbos);
        }

        GLenum usage = dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
        
        if (maxVertices_ < vertices.size()) {
            maxVertices_ = vertices.size();
            glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::VERTEX]);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], usage);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::NORMAL]);
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), &normals[0], usage);

            if (hasUVBuffer()) {
                glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::UV]);
                glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], usage);
            }
        } else {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::VERTEX]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec3), &vertices[0]);

            glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::NORMAL]);
            glBufferSubData(GL_ARRAY_BUFFER, 0, normals.size() * sizeof(glm::vec3), &normals[0]);

            if (hasUVBuffer()) {
                glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::UV]);
                glBufferSubData(GL_ARRAY_BUFFER, 0, uvs.size() * sizeof(glm::vec2), &uvs[0]);
            }
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos[vboName::INDEX]);
        if (maxIndices_ < indices.size()) {
            maxIndices_ = indices.size();
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], usage);
        } else {
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(unsigned int), &indices[0]);
        }

        if (freeCPUCopy)
            Free(false, true);
    }

	void Mesh::Free(bool gpuCopy, bool cpuCopy) {
		if (cpuCopy) {
            numVertices_ = vertices.size();
            numIndices_ = indices.size();
            vertices.clear();
            normals.clear();
            uvs.clear();
            indices.clear();
        }

		if (gpuCopy && gpuCopyCreated()) {
            GLuint numBuffers = 2;
            if (hasUVBuffer())
                ++numBuffers;
            if (hasIndexBuffer())
                ++numBuffers;
            glDeleteBuffers(numBuffers, vbos);

            numVertices_ = numIndices_ = maxVertices_ = maxIndices_ = 0;
		}
	}

} // namespace Progression