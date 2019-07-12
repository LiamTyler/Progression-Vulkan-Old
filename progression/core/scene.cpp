#include "core/scene.hpp"
#include <fstream>
#include <sstream>
#include "resource/resource_manager.hpp"
#include "utils/logger.hpp"
#include "fileIO.hpp"
#include "graphics/render_system.hpp"
// #include "graphics/shadow_map.hpp"


namespace Progression {

    static bool parseCamera(Scene* scene, std::ifstream& in) {
        Camera& camera = scene->camera;
        std::string line;
        float tmp;

        fileIO::parseLineKeyVal(in, "position", camera.position);
        fileIO::parseLineKeyVal(in, "rotation", camera.rotation);
        fileIO::parseLineKeyVal(in, "fov", tmp);
        camera.setFOV(glm::radians(tmp));
        std::getline(in, line);
        std::stringstream ss(line);
        std::string first;
        ss >> first;
        PG_ASSERT(first == "aspect");
        float width, height;
        char colon;
        ss >> width >> colon >> height;
        camera.setAspectRatio(width / height);
        fileIO::parseLineKeyVal(in, "near-plane", tmp);
        camera.setNearPlane(tmp);
        fileIO::parseLineKeyVal(in, "far-plane", tmp);
        camera.setFarPlane(tmp);

        return !in.fail();
    }

    static bool parseLight(Scene* scene, std::ifstream& in) {
        Light light;
        std::string line = " ";
        
        PG_UNUSED(scene);
        PG_UNUSED(in);

        return !in.fail();
    }

    Scene::Scene() :
        backgroundColor(glm::vec3(0)),
        ambientColor(glm::vec3(0))
    {
    }

    Scene::~Scene() {
        for (const auto& l : lights_)
            delete l;
    }

    Scene* Scene::load(const std::string& filename) {
        Scene* scene = new Scene;

        std::ifstream in(filename);
        if (!in) {
            LOG_ERR("Could not open scene:", filename);
            return nullptr;
        }
        std::string line;
        while (std::getline(in, line)) {
            if (line == "")
                continue;
            if (line[0] == '#')
                continue;
            if (line == "Resource-file") {
                std::getline(in, line);
                std::stringstream ss(line);
                std::string fname;
                ss >> fname;
                ResourceManager::loadResourceFileAsync(fname);
            } else if (line == "Camera") {
                if (!parseCamera(scene, in)) {
                    LOG_ERR("Could not parse camera in scene file");
                    delete scene;
                    return nullptr;
                }
            } else if (line == "Light") {
                if (!parseLight(scene, in)) {
                    LOG_ERR("Could not parse light in scene file");
                    delete scene;
                    return nullptr;
                }
            } else if (line == "AmbientLight") {
                // next line should be "color _ _ _"
                std::getline(in, line);
                std::stringstream ss(line);
                std::string name;
                ss >> name;
                ss >> scene->ambientColor;
            } else if (line == "BackgroundColor") {
                // next line should be "color _ _ _"
                std::getline(in, line);
                std::stringstream ss(line);
                std::string name;
                ss >> name;
                ss >> scene->backgroundColor;
            }
        }

        in.close();

        return scene;
    }

} // namespace Progression
