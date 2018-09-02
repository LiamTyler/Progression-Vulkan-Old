#include "progression.h"
// #include "primary_canvas.h"
#include "tinyobjloader/tiny_obj_loader.h"
#include <unordered_map>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp> 

using namespace Progression;

std::string rootDirectory;
// PrimaryCanvas* primaryCanvas;





class Model2 {
public:
    Model2() = default;

    std::vector<Mesh*> subMeshes;
};

class Vertex {
public:
    Vertex(const glm::vec3& vert, const glm::vec3& norm, const glm::vec2& tex) :
        vertex(vert),
        normal(norm),
        uv(tex) {}

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


bool LoadModel(const std::string& filename, Model2& model, bool free = true) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename.c_str());

    if (!err.empty()) { // `err` may contain warning message.
        std::cerr << err << std::endl;
    }

    if (!ret) {
        std::cout << "Failed to load the input file: " << std::endl;
        return false;
    }

    std::vector<Material> pgMaterials;
    for (const auto& mat : materials)
    {
        glm::vec3 ambient(mat.ambient[0], mat.ambient[1], mat.ambient[2]);
        glm::vec3 diffuse(mat.diffuse[0], mat.diffuse[1], mat.diffuse[2]);
        glm::vec3 specular(mat.specular[0], mat.specular[1], mat.specular[2]);
        float shinyness = mat.shininess;
        pgMaterials.emplace_back(ambient, diffuse, specular, shinyness);
    }
    // add default material to the end of the list for faces with material_id == -1
    pgMaterials.emplace_back();


    // Loop over shapes
    std::cout << "num shapes: " << shapes.size() << std::endl;
    std::cout << "num materials: " << materials.size() << std::endl;

    

    std::vector<Mesh*> pgMeshes;

    for (const auto& shape : shapes) {

        std::cout << "num faces: " << shape.mesh.num_face_vertices.size() << std::endl;
        std::unordered_map<int, std::vector<Vertex>> matToMeshMap;
        
        // Loop over faces(polygon)
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {            
            auto& data = matToMeshMap[shape.mesh.material_ids[f]];

            // Loop over vertices in the face. Each face should have 3 vertices from the LoadObj triangulation
            for (size_t v = 0; v < 3; v++) {

                // access to vertex
                tinyobj::index_t idx = shape.mesh.indices[3 * f + v];
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];

                data.emplace_back(glm::vec3(vx, vy, vz), glm::vec3(nx, ny, nz), glm::vec2(tx, ty));
            }
        }

        /// remove duplicate vertices from each mesh
        for (const auto& pair : matToMeshMap) {
            Material* mat = new Material;
            if (pair.first == -1)
                *mat = pgMaterials[pgMaterials.size() - 1];
            else
                *mat = pgMaterials[pair.first];

            const auto& data = pair.second;
            glm::vec3* vertices = new glm::vec3[data.size()];
            glm::vec3* normals = new glm::vec3[data.size()];
            glm::vec2* uvs = new glm::vec2[data.size()];
            unsigned int* indices = new unsigned int[data.size()];
            for (int i = 0; i < data.size(); ++i) {
                vertices[i] = data[i].vertex;
                normals[i] = data[i].normal;
                uvs[i] = data[i].uv;
                indices[i] = i;
            }
            Mesh* m = new Mesh(data.size(), data.size() / 3, vertices, normals, uvs, indices);
            m->UploadToGPU(free);
            pgMeshes.push_back(m);
        }
    }

    model.subMeshes = pgMeshes;

    return true;
}

namespace Progression {
    class RenderComponent : public Component {
    public:
        RenderComponent() = default;
        virtual void Start() = 0;
        virtual void Update(float dt) = 0;
        virtual void Stop() = 0;
        bool visible;
    };

    class MeshRenderer2 : public RenderComponent {
    public:
        MeshRenderer2() = default;
        virtual void Start() {};
        virtual void Update(float dt) {};
        virtual void Stop() {}
    };
}

class Scene {
public:
    explicit Scene(unsigned int maxObjects) :
        maxGameObjects_(maxObjects)
    {

    }

    ~Scene() {
        for (const auto& o : gameObjects_)
            delete o;
    }

    void AddGameObject(GameObject* o) {
        gameObjects_.push_back(o);
    }

    void GenerateVisibilityList(std::vector<GameObject*>& visList) {
        visList.clear();
        //visList.reserve(gameObjects_.size());
        for (const auto& o : gameObjects_) {
            visList.push_back(o);
            /*
            auto renderer = o->GetComponent<RenderComponent>();
            if (renderer && renderer->visible)
                visList.push_back(o);
            */
        }
    }

private:
    unsigned int maxGameObjects_;
    std::vector<GameObject*> gameObjects_;
};

//class RenderSystem {
//public:
//    RenderSystem() = default;
//    ~RenderSystem() = default;
//
//    std::vector<RenderSubSystems*> subSystems;
//};

// argv[1] = path of the root directory
int main(int argc, char* argv[]) {

    if (argc < 2) {
        std::cout << "Please pass in the path of the root directory as the first argument" << std::endl;
        return 0;
    }
    rootDirectory = argv[1];

    auto& conf = PG::config::Config(rootDirectory + "configs/default.yaml");

    PG::Window::Init(conf);
    PG::Time::Init(conf);
    Input::Init(conf);

    auto screen = PG::Window::getUIScreen();
    screen->setVisible(true);
    screen->performLayout();

    Camera camera = Camera(Transform(
        glm::vec3(0, 0, 5),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 1, 0)));
    camera.AddComponent<UserCameraComponent>(new UserCameraComponent(&camera));
    

    GameObject go;
    go.AddComponent<RenderComponent>(new MeshRenderer2);

    Model2 model;
    
    if (!LoadModel(rootDirectory + "resources/models/chalet2.obj", model, true)) {
        std::cout << "Failed to load the model, exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
    else {
        std::cout << "loaded successfully" << std::endl;
    }
    
    Shader phongShader(
        rootDirectory + "resources/shaders/regular_phong.vert",
        rootDirectory + "resources/shaders/regular_phong.frag");

    //Texture modelTex = Texture(rootDirectory + "resources/textures/chalet.jpg");

    Mesh* mesh = model.subMeshes[0];

    GLuint modelVAO;
    glGenVertexArrays(1, &modelVAO);
    glBindVertexArray(modelVAO);
    GLuint* vbos_ = mesh->getBuffers();
    
    glBindBuffer(GL_ARRAY_BUFFER, vbos_[0]);
    glEnableVertexAttribArray(phongShader["vertex"]);
    glVertexAttribPointer(phongShader["vertex"], 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbos_[1]);
    glEnableVertexAttribArray(phongShader["normal"]);
    glVertexAttribPointer(phongShader["normal"], 3, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ARRAY_BUFFER, vbos_[2]);
    glEnableVertexAttribArray(phongShader["inTexCoord"]);
    glVertexAttribPointer(phongShader["inTexCoord"], 2, GL_FLOAT, GL_FALSE, 0, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbos_[3]);

    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(Window::getGLFWHandle(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Game loop
    while (!glfwWindowShouldClose(Window::getGLFWHandle())) {
        PG::Window::StartFrame();
        PG::Input::PollEvents();

        float dt = Time::deltaTime();
        camera.Update(dt);

        const auto& bgColor = PG::Window::getBackgroundColor();
        glClearColor(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
        //glClearColor(0, 1, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

        
        phongShader.Enable();
        glBindVertexArray(modelVAO);

        glm::mat4 P = camera.GetP();
        glm::mat4 V = camera.GetV();
        glUniformMatrix4fv(phongShader["projectionMatrix"], 1, GL_FALSE, glm::value_ptr(P));

        glUniform3fv(phongShader["Ia"], 1, glm::value_ptr(glm::vec3(0)));
        glUniform3fv(phongShader["Id"], 1, glm::value_ptr(glm::vec3(1)));
        glUniform3fv(phongShader["Is"], 1, glm::value_ptr(glm::vec3(.3)));
        glm::vec3 lEye = glm::vec3(V * glm::vec4(glm::vec3(1, -1, -1), 0));
        glUniform3fv(phongShader["lightInEyeSpace"], 1, glm::value_ptr(lEye));

        //gameObj.GetComponent<ModelRenderer>()->Render(camera);

        glUniform3fv(phongShader["ka"], 1, glm::value_ptr(glm::vec3(0)));
        glUniform3fv(phongShader["kd"], 1, glm::value_ptr(glm::vec3(1)));
        glUniform3fv(phongShader["ks"], 1, glm::value_ptr(glm::vec3(.2)));
        glUniform1f(phongShader["specular"], 50);
        glUniform1i(phongShader["textured"], true);
        glActiveTexture(GL_TEXTURE0);
        // glBindTexture(GL_TEXTURE_2D, modelTex.GetHandle());
        glUniform1i(phongShader["diffuseTex"], 0);
        // send model and normal matrices

        glm::mat4 modelMatrix(1);
        glm::mat4 MV = camera.GetV() * modelMatrix;
        glm::mat4 normalMatrix = glm::transpose(glm::inverse(MV));
        glUniformMatrix4fv(phongShader["modelViewMatrix"], 1, GL_FALSE, glm::value_ptr(MV));
        glUniformMatrix4fv(phongShader["normalMatrix"], 1, GL_FALSE, glm::value_ptr(normalMatrix));
        glDrawElements(GL_TRIANGLES, mesh->getNumTriangles() * 3, GL_UNSIGNED_INT, 0);
        

        // Draw UI & primary canvas
        screen->drawContents();
        screen->drawWidgets();

        PG::Window::EndFrame();
    }

    Input::Free();
    Time::Free();
    PG::Window::Free();

    return 0;
}
