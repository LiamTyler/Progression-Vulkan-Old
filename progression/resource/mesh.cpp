#include "resource/mesh.hpp"
#include "graphics/graphics_api.hpp"
#include "utils/logger.hpp"
#include "meshoptimizer/src/meshoptimizer.h"
#include "utils/serialize.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

class Vertex {
public:
    Vertex() :
        vertex(glm::vec3(0)),
        normal(glm::vec3(0)),
        uv(glm::vec3(0))
    {
    }

    Vertex(const glm::vec3& vert, const glm::vec3& norm, const glm::vec2& tex) :
        vertex(vert),
        normal(norm),
        uv(tex)
    {
    }

    bool operator==(const Vertex& other) const {
        return vertex == other.vertex && normal == other.normal && uv == other.uv;
    }

    glm::vec3 vertex;
    glm::vec3 normal;
    glm::vec2 uv;
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.vertex) ^
                        (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.uv) << 1);
        }
    };
}        


namespace Progression {

    Mesh::Mesh() {
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
        gpuDataCreated = std::move(mesh.gpuDataCreated);

        vertexBuffer = std::move(mesh.vertexBuffer);
        indexBuffer  = std::move(mesh.indexBuffer);
        mesh.vertexBuffer = -1;
        mesh.indexBuffer  = -1;

        vao = std::move(mesh.vao);
        mesh.vao = (GLuint) -1;

        return *this;
    }

    void Mesh::optimize() {
        if (vertices.size() == 0) {
            LOG_ERR("Trying to optimize a mesh with no vertices. Did you free them after uploading to the GPU?");
            return;
        }
        // collect everything back into interleaved data
        std::vector<Vertex> opt_vertices;
        for (size_t i = 0; i < vertices.size(); ++i) {
            Vertex v;
            v.vertex = vertices[i];
            if (normals.size())
                v.normal = normals[i];
            if (uvs.size())
                v.uv = uvs[i];

            opt_vertices.push_back(v);
        }

        // const size_t kCacheSize = 16;
        // meshopt_VertexCacheStatistics vcs = meshopt_analyzeVertexCache(&mesh.indices[0], mesh.indices.size(),
        //         mesh.vertices.size(), kCacheSize, 0, 0);
        // meshopt_OverdrawStatistics os = meshopt_analyzeOverdraw(&mesh.indices[0], mesh.indices.size(),
        //         &vertices[0].vertex.x, mesh.vertices.size(), sizeof(Vertex));
        // meshopt_VertexFetchStatistics vfs = meshopt_analyzeVertexFetch(&mesh.indices[0], mesh.indices.size(),
        //         mesh.vertices.size(), sizeof(Vertex));
        // LOG("Before:");
        // LOG("ACMR: ", vcs.acmr, ", ATVR: ", vcs.atvr, ", avg overdraw: ", os.overdraw, " avg # fetched: ", vfs.overfetch);

        // vertex cache optimization should go first as it provides starting order for overdraw
        meshopt_optimizeVertexCache(&indices[0], &indices[0], indices.size(), opt_vertices.size());

        // reorder indices for overdraw, balancing overdraw and vertex cache efficiency
        const float kThreshold = 1.01f; // allow up to 1% worse ACMR to get more reordering opportunities for overdraw
        meshopt_optimizeOverdraw(&indices[0], &indices[0], indices.size(),
                                 &opt_vertices[0].vertex.x, opt_vertices.size(), sizeof(Vertex), kThreshold);

        // vertex fetch optimization should go last as it depends on the final index order
        meshopt_optimizeVertexFetch(&opt_vertices[0].vertex.x, &indices[0], indices.size(),
                                    &opt_vertices[0].vertex.x, opt_vertices.size(), sizeof(Vertex));

        // vcs = meshopt_analyzeVertexCache(&indices[0], indices.size(), vertices.size(), kCacheSize, 0, 0);
        // os = meshopt_analyzeOverdraw(&indices[0], indices.size(), &vertices[0].vertex.x, vertices.size(), sizeof(Vertex));
        // vfs = meshopt_analyzeVertexFetch(&indices[0], indices.size(), vertices.size(), sizeof(Vertex));
        // LOG("After:");
        // LOG("ACMR: ", vcs.acmr, ", ATVR: ", vcs.atvr, ", avg overdraw: ", os.overdraw, " avg # fetched: ", vfs.overfetch);

        // collect back into mesh structure
        for (size_t i = 0; i < opt_vertices.size(); ++i) {
            const auto& v = opt_vertices[i];
            vertices[i] = v.vertex;
            if (normals.size())
                normals[i] = v.normal;
            if (uvs.size())
                uvs[i] = v.uv;
        }
    }

    // TODO: dynamic meshes + usage
    void Mesh::uploadToGpu(bool freeCPUCopy) {
        if (!gpuDataCreated) {
            gpuDataCreated = true;
            graphicsApi::createBuffers(&vertexBuffer, 1);
            graphicsApi::createBuffers(&indexBuffer, 1);
            vao = graphicsApi::createVao();
        }

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

    bool Mesh::saveToFastFile(std::ofstream& out) const {
        if (!vertices.size()) {
            LOG_ERR("No vertex data to save. Was this mesh accidentally uploaded to the GPU + freed?");
            return false;
        }

        serialize::write(out, vertices);
        serialize::write(out, normals);
        serialize::write(out, uvs);
        serialize::write(out, indices);

        return !out.fail();
    }

    bool Mesh::loadFromFastFile(std::ifstream& in) {
        gpuDataCreated = false;
        serialize::read(in, vertices);
        serialize::read(in, normals);
        serialize::read(in, uvs);
        serialize::read(in, indices);

        return !in.fail();
    }

} // namespace Progression
