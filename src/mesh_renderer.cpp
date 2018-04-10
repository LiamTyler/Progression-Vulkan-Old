#include "include/mesh_renderer.h"

MeshRenderer::MeshRenderer(Shader* sh, Mesh* m) {
    shader_ = sh;
    mesh_ = m;
}

MeshRenderer::~MeshRenderer() {
}

void MeshRenderer::Start() {
    Shader& shader = *shader_;
    
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    vbos_.resize(3);
    glGenBuffers(3, &vbos_[0]);
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glBufferData(GL_ARRAY_BUFFER, mesh_->numVertices * sizeof(glm::vec3),
            mesh_->vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(shader["vertex"]);
    glVertexAttribPointer(shader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbos_[1]);
    glBufferData(GL_ARRAY_BUFFER, mesh_->numVertices * sizeof(glm::vec3),
            mesh_->normals, GL_STATIC_DRAW);
    glEnableVertexAttribArray(shader["normal"]);
    glVertexAttribPointer(shader["normal"], 3, GL_FLOAT, GL_FALSE, 0, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh_->numTriangles * sizeof(glm::ivec3),
        mesh_->indices, GL_STATIC_DRAW);
}

void MeshRenderer::Update(float dt) {
}

void MeshRenderer::Stop() {
    glDeleteBuffers(vbos_.size(), &vbos_[0]);
    glDeleteVertexArrays(1, &vao_);
}

void MeshRenderer::Render(const Camera& camera) {
    Shader& shader = *shader_;
    glBindVertexArray(vao_);
    // send material
    /*
    glUniform4fv(shader["ka"], 1, value_ptr(material_->ka));
    glUniform4fv(shader["kd"], 1, value_ptr(material_->kd));
    glUniform4fv(shader["ks"], 1, value_ptr(material_->ks));
    glUniform1f(shader["specular"], material_->specular);
    if (material_->diffuseTex) {
        glUniform1i(shader["textured"], true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, material_->diffuseTex->GetHandle());
        glUniform1i(shader["diffuseTex"], 0);
    }
    */

    // send model and normal matrices
    glm::mat4 modelMatrix = gameObject->transform.GetModelMatrix();
    glm::mat4 MV = camera.GetV() * modelMatrix;
    glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
    glUniformMatrix4fv(shader["modelViewMatrix"], 1, GL_FALSE, value_ptr(MV));
    glUniformMatrix4fv(shader["normalMatrix"], 1, GL_FALSE, value_ptr(normalMatrix));

    // draw
    glDrawElements(GL_TRIANGLES, mesh_->numTriangles * 3, GL_UNSIGNED_INT, 0);
}

