#include "core/scene.hpp"
#include <fstream>
#include <sstream>
#include "utils/logger.hpp"
#include "utils/fileIO.hpp"
#include "core/ecs.hpp"
#include "resource/resource_manager.hpp"
#include "graphics/lights.hpp"

#include "components/model_renderer_component.hpp"

using namespace Progression;

static bool parseResourcefile(std::istream& in) {
    std::string fname;
    fileIO::parseLineKeyVal(in, "filename", fname);
    return !in.fail() && ResourceManager::loadResourceFile(PG_RESOURCE_DIR + fname);
}

static bool parseCamera(Scene* scene, std::istream& in) {
    Camera& camera = scene->camera;
    std::string line;
    float tmp;

    fileIO::parseLineKeyVal(in, "position", camera.position);
    glm::vec3 rotInDegrees;
    fileIO::parseLineKeyVal(in, "rotation", rotInDegrees);
    camera.rotation = glm::radians(rotInDegrees);
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

    camera.updateOrientationVectors();
    camera.updateViewMatrix();

    return !in.fail();
}

static bool parseLight(Scene* scene, std::istream& in) {
    Light light;
    std::string tmp;

    fileIO::parseLineKeyVal(in, "name", light.name);
    fileIO::parseLineKeyVal(in, "type", tmp);
    PG_ASSERT(tmp == "point" || tmp == "spot" || tmp == "directional");
    light.type = tmp == "point" ? Light::Type::POINT : tmp == "spot" ? Light::Type::SPOT : Light::Type::DIRECTIONAL;
    light.enabled = fileIO::parseLineKeyBool(in, "enabled");
    fileIO::parseLineKeyVal(in, "color", light.color);
    fileIO::parseLineKeyVal(in, "intensity", light.intensity);
    fileIO::parseLineKeyValOptional(in, "position", light.position);
    fileIO::parseLineKeyValOptional(in, "direction", light.direction);
    light.direction = glm::normalize(light.direction);
    fileIO::parseLineKeyValOptional(in, "radius", light.radius);
    fileIO::parseLineKeyValOptional(in, "innerCutoff", light.innerCutoff);
    fileIO::parseLineKeyValOptional(in, "outerCutoff", light.outerCutoff);

    scene->addLight(new Light(light));
    return !in.fail();
}

static bool parseEntity(std::istream& in) {
    auto e = ECS::entity::create();
    ECS::EntityData* data = ECS::entity::data(e);
    fileIO::parseLineKeyValOptional(in, "name", data->name);
    std::string parentName;
    fileIO::parseLineKeyValOptional(in, "parent", parentName);
    if (parentName.empty())
        data->parentID = ECS::INVALID_ENTITY_ID;
    fileIO::parseLineKeyVal(in, "position", data->transform.position);
    fileIO::parseLineKeyVal(in, "rotation", data->transform.rotation);
    data->transform.rotation = glm::radians(data->transform.rotation);
    fileIO::parseLineKeyVal(in, "scale", data->transform.scale);
    data->isStatic = fileIO::parseLineKeyBool(in, "static");
    int numComponents;
    fileIO::parseLineKeyVal(in, "numComponents", numComponents);
    for (int i = 0; i < numComponents; ++i) {
        std::string component;
        fileIO::parseLineKeyVal(in, "Component", component);
        if (component == "ModelRenderer") {
            auto comp = ECS::component::create<ModelRenderComponent>(e);
            parseModelRendererComponentFromFile(in, *comp);
        } else {
            LOG_WARN("Unrecognized component: '", component, "'");
        }
    }

    return !in.fail();
}

static bool parseBackgroundColor(Scene* scene, std::istream& in) {
    fileIO::parseLineKeyVal(in, "color", scene->backgroundColor);

    return !in.fail();
}

static bool parseAmbientColor(Scene* scene, std::istream& in) {
    fileIO::parseLineKeyVal(in, "color", scene->ambientColor);

    return !in.fail();
}

namespace Progression {

Scene::Scene() : backgroundColor(glm::vec3(0))
{
}

Scene::~Scene() {
    for (auto l : lights_)
        delete l;
}

void Scene::addLight(Light* light) {
    switch (light->type) {
        case Light::Type::POINT:
            ++numPointLights_;
            break;
        case Light::Type::SPOT:
            ++numSpotLights_;
            break;
        case Light::Type::DIRECTIONAL:
            ++numDirectionalLights_;
            break;
    }
    lights_.push_back(light);
}

void Scene::removeLight(Light* light) {
    lights_.erase(std::remove(lights_.begin(), lights_.end(), light), lights_.end());
}

void Scene::sortLights() {
    std::sort(lights_.begin(), lights_.end(), [](Light* lhs, Light* rhs) {
        return lhs->type < rhs->type;
    });
}

#define PARSE_SCENE_ELEMENT(name, function) \
else if (line == name) { \
    if (!function) { \
        LOG_ERR("Could not parse '" #name "' in scene file"); \
        delete scene; \
        return nullptr; \
    } \
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
        if (line == "" || line[0] == '#') {
            continue;
        }
        PARSE_SCENE_ELEMENT("Resourcefile", parseResourcefile(in))
        PARSE_SCENE_ELEMENT("Camera", parseCamera(scene, in))
        PARSE_SCENE_ELEMENT("Light", parseLight(scene, in))
        PARSE_SCENE_ELEMENT("Entity", parseEntity(in))
        PARSE_SCENE_ELEMENT("BackgroundColor", parseBackgroundColor(scene, in))
        PARSE_SCENE_ELEMENT("AmbientColor", parseAmbientColor(scene, in))
        else {
            LOG_WARN("Unrecognized scene file entry: ", line);
        }
    }

    ResourceManager::waitUntilLoadComplete();

    in.close();

    return scene;
}

} // namespace Progression
