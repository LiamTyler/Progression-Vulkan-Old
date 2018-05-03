#pragma once

#include "include/utils.h"
#include "include/shader.h"
#include "include/camera.h"

class Skybox {
    public:
        Skybox(const std::string& vertShader, const std::string& fragShader);
        ~Skybox();
        bool Load(
                const std::string& right,
                const std::string& left,
                const std::string& top,
                const std::string& bottom,
                const std::string& front,
                const std::string& back);
        bool Load(std::vector<std::string>& faces);

        void Render(const Camera& camera);

        GLuint GetTextureID() { return skyboxTextureID_; }
        const Shader& GetShader() { return shader_; }
        
    private:
        GLuint skyboxTextureID_;
        GLuint cubeVao_;
        GLuint cubeVbo_;
        Shader shader_;
        bool loaded_;
};
