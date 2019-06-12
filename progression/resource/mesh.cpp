#include "graphics/mesh.hpp"
#include "graphics/graphics_api.hpp"

namespace Progression {

    Mesh::Mesh() {
        for (GLuint i = 0; i < vboName::TOTAL_VBOS; ++i)
            vbos[i] = (GLuint) -1;
    }

    Mesh::~Mesh() {
        if (gpuCopyCreated()) {
            GLuint numBuffers = 2;
            if (hasUVBuffer())
                ++numBuffers;
            if (hasIndexBuffer())
                ++numBuffers;

            graphicsApi::deleteBuffers(vbos, numBuffers);
            if (vao != (GLuint) -1)
                graphicsApi::deleteVao(vao);
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

        for (GLuint i = 0; i < vboName::TOTAL_VBOS; ++i) {
            vbos[i] = mesh.vbos[i];
            mesh.vbos[i] = (GLuint) -1;
        }

        vao = std::move(mesh.vao);
        mesh.vao = (GLuint) -1;

        return *this;
    }

    void Mesh::UploadToGPU(bool freeCPUCopy) {
        bool created = false;
        if (!gpuCopyCreated()) {
            GLuint numBuffers = 2;
            if (uvs.size())
                ++numBuffers;

            if (indices.size())
                ++numBuffers;

            graphicsApi::createBuffers(vbos, numBuffers);
            vao = graphicsApi::createVao();
            created = true;
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

        if (created) {
            glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::VERTEX]);
            graphicsApi::describeAttribute(0, 3, GL_FLOAT);
            glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::NORMAL]);
            graphicsApi::describeAttribute(1, 3, GL_FLOAT);
            if (hasUVBuffer()) {
                glBindBuffer(GL_ARRAY_BUFFER, vbos[vboName::UV]);
                graphicsApi::describeAttribute(2, 2, GL_FLOAT);
            }
        }

        if (freeCPUCopy)
            Free(false, true);
    }

    void Mesh::Free(bool gpuCopy, bool cpuCopy) {
        if (cpuCopy) {
            numVertices_ = vertices.size();
            numIndices_ = indices.size();
            vertices.shrink_to_fit();
            normals.shrink_to_fit();
            uvs.shrink_to_fit();
            indices.shrink_to_fit();
        }

        if (gpuCopy && gpuCopyCreated()) {
            GLuint numBuffers = 2;
            if (hasUVBuffer())
                ++numBuffers;
            if (hasIndexBuffer())
                ++numBuffers;
            graphicsApi::deleteBuffers(vbos, numBuffers);
            graphicsApi::deleteVao(vao);

            numVertices_ = numIndices_ = maxVertices_ = maxIndices_ = 0;
        }
    }

} // namespace Progression
