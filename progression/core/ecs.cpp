#include "core/ecs.hpp"
#include "utils/logger.hpp"

static Progression::ECS::EntityManager s_entityManager;

namespace Progression {

namespace ECS {

    std::vector<ComponentPool> components_;
    uint32_t BaseFamily::typeCounter = 0;

    EntityManager::EntityManager(uint32_t reserveSize) {
        entityVersions.reserve(reserveSize);
        entityData.reserve(reserveSize);
    }

    void EntityManager::clear() {
        freeIndices.clear();
        entityVersions.clear();
        entityData.clear();
    }

    Entity EntityManager::create() {
        entity_index_type index = 0;
        entity_version_type version = 0;
        if (freeIndices.size() > MINIMUM_FREE_INDICIES) {
            index = freeIndices.front();
            freeIndices.pop_front();
            version = entityVersions[index];
        }
        else {
            index = entityVersions.size();
            entityVersions.push_back(0);
            entityData.emplace_back();
        }

        return Entity(index, version);
    }

    bool EntityManager::alive(const Entity e) const {
        LOG("index = ", e.index(), ", version = ", e.version(), ", size = ", entityVersions.size());
        return e.index() < entityVersions.size() && entityVersions[e.index()] == e.version();
    }

    void EntityManager::destroy(const Entity e) {
        PG_ASSERT(alive(e));
        const auto idx = e.index();
        ++entityVersions[idx];
        freeIndices.push_back(idx);
        for (uint32_t cTypeID = 0; cTypeID < MAX_COMPONENTS; ++cTypeID) {
            if (entityData[idx].hasComponents[cTypeID])
                ECS::component::destroy(cTypeID, e);
        }
    }

    Entity EntityManager::get(const std::string& name) const {
        size_t index = 0;
        while (index < entityData.size() && name != entityData[index].name) {
            ++index;
        }
        if (index == entityData.size())
            return INVALID_ENTITY_ID;
        return Entity(index, entityVersions[index]);
    }

    EntityData* EntityManager::getData(const Entity& e) {
        PG_ASSERT(alive(e));
        return &entityData[e.index()];
    }


    namespace entity {

        EntityData* data(const entity_type& id) {
            return s_entityManager.getData(id);
        }

        Entity create() {
            return s_entityManager.create();
        }

        bool alive(const Entity e) {
            return s_entityManager.alive(e);
        }

        Entity get(const std::string& name) {
            return s_entityManager.get(name);
        }

        void destroy(const Entity e) {
            return s_entityManager.destroy(e);
        }

    } // namespace entity


    void init() {
        components_.resize(MAX_COMPONENTS);
    }

    void shutdown() {
        s_entityManager.clear();
        components_.clear();
    }
        

} // namespace ECS

} // namespace Progression