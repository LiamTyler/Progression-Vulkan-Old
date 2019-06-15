#include "resource/mesh.hpp"
#include "graphics/graphics_api.hpp"

namespace Progression {

    Mesh::Mesh() {
        graphicsApi::createBuffers(&vertexBuffer, 1);
        graphicsApi::createBuffers(&indexBuffer, 1);
        vao = graphicsApi::createVao();
    }

    Mesh::~Mesh() {
        if (vertexBuffer != (GLuint) -1)
            graphicsApi::deleteBuffers(&vertexBuffer, 1);
        if (indexBuffer != (GLuint) -1)
            graphicsApi::deleteBuffers(&indexBuffer, 1);
        if (vao != (GLuint) -1)
            graphicsApi::deleteVao(vao);
    }

    Mesh::Mesh(Mesh&& mesh) {
        *this = std::move(mesh);
    }

    Mesh& Mesh::operator=(Mesh&& mesh) {
        vertices       = std::move(mesh.vertices);
        normals        = std::move(mesh.normals);
        uvs            = std::move(mesh.uvs);
        indices        = std::move(mesh.indices);
        numVertices_   = std::move(mesh.numVertices_);
        numIndices_    = std::move(mesh.numIndices_);
        normalOffset_  = std::move(mesh.normalOffset_);
        textureOffset_ = std::move(mesh.textureOffset_);

        vertexBuffer = std::move(mesh.vertexBuffer);
        indexBuffer  = std::move(mesh.indexBuffer);
        mesh.vertexBuffer = -1;
        mesh.indexBuffer  = -1;

        vao = std::move(mesh.vao);
        mesh.vao = (GLuint) -1;

        return *this;
    }

    // TODO: dynamic meshes + usage
    void Mesh::uploadToGpu(bool freeCPUCopy) {
        GLenum usage = GL_STATIC_DRAW;
        numVertices_ = vertices.size();
        numIndices_  = indices.size();
        normalOffset_ = numVertices_ * sizeof(glm::vec3);
        textureOffset_ = normalOffset_ + normals.size() * sizeof(glm::vec3);

        graphicsApi::bindVao(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(
                GL_ARRAY_BUFFER,
                (vertices.size() + normals.size()) * sizeof(glm::vec3) + uvs.size() * sizeof(glm::vec2),
                NULL, 
                usage);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(glm::vec3), &vertices[0]);
        graphicsApi::describeAttribute(0, 3, GL_FLOAT);

        if (normals.size()) {
            glBufferSubData(GL_ARRAY_BUFFER, normalOffset_, normals.size() * sizeof(glm::vec3), &normals[0]);
            graphicsApi::describeAttribute(1, 3, GL_FLOAT, 0, normalOffset_);
        }
        if (uvs.size()) {
            glBufferSubData(GL_ARRAY_BUFFER, textureOffset_, uvs.size() * sizeof(glm::vec2), &uvs[0]);
            graphicsApi::describeAttribute(2, 2, GL_FLOAT, 0, textureOffset_);
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], usage);

        free(false, freeCPUCopy);
    }

    void Mesh::free(bool gpuCopy, bool cpuCopy) {
        if (cpuCopy) {
            numVertices_ = vertices.size();
            numIndices_ = indices.size();
            vertices.shrink_to_fit();
            normals.shrink_to_fit();
            uvs.shrink_to_fit();
            indices.shrink_to_fit();
        }

        if (gpuCopy) {
            graphicsApi::deleteBuffers(&vertexBuffer, 1);
            graphicsApi::deleteBuffers(&indexBuffer, 1);
            graphicsApi::deleteVao(vao);

            numVertices_  = numIndices_ = 0;
            normalOffset_ = textureOffset_ = -1;
        }
    }

} // namespace Progression
