#include <iostream>
#include <vector>
#include <iterator>
#include <memory>
#include <deque>
#include <cstring>
#include <utility>
#include <type_traits>
#include <functional>
#include <bitset>
#include "core/common.hpp"
#include "core/transform.hpp"

namespace Progression {

namespace ECS {

class BaseFamily {
public:
    static inline uint32_t typeCounter_;
};

template <typename Derived>
class Family : public BaseFamily {
public:
    static uint32_t id() {
        static uint32_t typeIndex = typeCounter_++;
        return typeIndex;
    }
};

using entity_type         = uint32_t;
using entity_index_type   = uint32_t;
using entity_version_type = uint8_t;
static constexpr auto ENTITY_INDEX_BITS = 24;
static constexpr entity_type ENTITY_INDEX_MASK = (1ull << ENTITY_INDEX_BITS) - 1;
static constexpr auto ENTITY_VERSION_BITS = 8;
static constexpr entity_type ENTITY_VERSION_MASK = (1ull << ENTITY_VERSION_BITS) - 1;
static constexpr uint32_t MAX_COMPONENTS = 64;

static constexpr entity_type INVALID_ENTITY_ID = ~0u;

// TODO: test to see if / how much gain from struct of arrays
class EntityData {
public:
    entity_type parentID = INVALID_ENTITY_ID;
    std::bitset<MAX_COMPONENTS> hasComponents;
    Transform transform;
};

// forward declarations

namespace entity {

    EntityData* data(const entity_type& id);

} // namespace entity

namespace component {

    void destroy(uint32_t componentFamilyID, const entity_type& e);

} // namespace component

class Entity {
    public:
        constexpr Entity() : id_(0) {}
        constexpr Entity(const entity_type e) : id_(e) {}
        constexpr Entity(const entity_index_type i, const entity_version_type v) :
            id_(((entity_type) v << ENTITY_INDEX_BITS) + i) {}

        constexpr entity_type id() const { return id_; }
        constexpr entity_index_type index() const { return id_ & ENTITY_INDEX_MASK; }
        constexpr entity_version_type version() const { return (id_ >> ENTITY_INDEX_BITS) & ENTITY_VERSION_MASK; }

        constexpr bool operator==(const Entity& e) const {
            return id_ == e.id_;
        }

        constexpr bool operator!=(const Entity& e) const {
            return !(id_ == e.id_);
        }

        constexpr operator entity_type() const {
            return id_;
        }

        EntityData* operator->() const {
            return ECS::entity::data(id_);
        }

    private:
        entity_type id_;
};

class EntityManager {
    static const auto MINIMUM_FREE_INDICIES = 128;

public:
    EntityManager(uint32_t reserveSize = 1024) {
        entityVersions.reserve(reserveSize);
        entityData.reserve(reserveSize);
    }

    void clear() {
        freeIndices.clear();
        entityVersions.clear();
        entityData.clear();
    }

    Entity create() {
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

    bool alive(const Entity e) const {
        PG_ASSERT(e.index() < entityVersions.size());
        return entityVersions[e.index()] == e.version();
    }

    void destroy(const Entity e) {
        PG_ASSERT(alive(e));
        const auto idx = e.index();
        ++entityVersions[idx];
        freeIndices.push_back(idx);
        for (uint32_t cTypeID = 0; cTypeID < MAX_COMPONENTS; ++cTypeID) {
            if (entityData[idx].hasComponents[cTypeID])
                ECS::component::destroy(cTypeID, e);
        }
    }

    EntityData* getData(const Entity& e) {
        PG_ASSERT(e.index() < entityData.size());
        return &entityData[e.index()];
    }

    std::deque<entity_index_type> freeIndices; // TODO: use a ring buffer or something w/contiguous mem
    std::vector<entity_version_type> entityVersions;
    std::vector<EntityData> entityData;
};

/**
 * Adding or deleting components does not invalidate iterators over the components.
 * It does however invalidate a direct reference or pointer to the components.
 * In order to avoid this, could create a component handle class that holds the index.
 * Could also instead use a memory pool + a free list, but that would make iteration
 * more complicated.
 */
class ComponentPool {
    static constexpr entity_index_type null = entity_index_type(-1);
    static constexpr uint32_t ENTITIES_PER_PAGE = 4096;
    public:
        // automatically reserve 4K memory for the components, and 1K entities
        ComponentPool() {
            components_.reserve(4096);
            direct_.reserve(1024);
        }
        ~ComponentPool() {
            if (components_.size()) {
                for (size_t i = 0; i < components_.size() / componentSize_; ++i) {
                    callDestructor_(&components_[i * componentSize_]);
                }
            }
            for (auto& page : indirect_)
                delete[] page;
        }

        ComponentPool(const ComponentPool& p) = delete;
        ComponentPool& operator=(const ComponentPool& p) = delete;

        ComponentPool(ComponentPool&& p) = default;
        ComponentPool& operator=(ComponentPool&& p) = default;

        template <typename Component>
        class iterator {
            friend class ComponentPool;

            using direct_type = std::vector<uint8_t>;
            using index_type = entity_type;

            iterator(direct_type* ref, const index_type idx)
                : direct(ref), index(idx) {}

            public:
                using difference_type   = entity_type;
                using value_type        = Component;
                using pointer           = value_type*;
                using reference         = value_type&;
                using iterator_category = std::random_access_iterator_tag;

                iterator() = default;

                iterator& operator++() {
                    --index;
                    return *this;
                }

                iterator operator++(int) {
                    iterator orig = *this;
                    ++(*this);
                    return orig;
                }

                iterator& operator--() {
                    ++index;
                    return *this;
                }

                iterator operator--(int) {
                    iterator orig = *this;
                    --(*this);
                    return orig;
                }

                iterator& operator+=(const difference_type diff) {
                    index -= diff;
                    return *this;
                }

                iterator operator+(const difference_type diff) const {
                    return iterator(direct, index - diff);
                }

                iterator& operator-=(const difference_type diff) {
                    return (*this += -diff);
                }

                iterator operator-(const difference_type diff) const {
                    return (*this + -diff);
                }

                reference operator[](const difference_type value) const {
                    const auto pos = index - value - 1;
                    return *((Component*) &((*direct)[pos * sizeof(Component)]));
                }

                bool operator==(const iterator iter) const {
                    return index == iter.index;
                }

                bool operator!=(const iterator iter) const {
                    return !(*this == iter);
                }

                bool operator<(const iterator& iter) const {
                    return index > iter.index;
                }
                bool operator>(const iterator& iter) const {
                    return index < iter.index;
                }
                bool operator<=(const iterator& iter) const {
                    return !(*this > iter);
                }
                bool operator>=(const iterator& iter) const {
                    return !(*this < iter);
                }

                pointer operator->() const {
                    const auto pos = index - 1;
                    return (Component*) &(*direct)[pos * sizeof(Component)];
                }

                reference operator*() const {
                    return *operator->();
                }

            private:
                direct_type* direct;
                index_type index;
        };

        template<typename Component>
        iterator<Component> begin() { return iterator<Component>(&components_, direct_.size()); }

        template<typename Component>
        iterator<Component> end() { return iterator<Component>(&components_, 0); }

        bool empty() const { return direct_.empty(); }
        size_t size() const { return direct_.size(); }
        size_t capacity() const { return direct_.capacity(); }

        template<typename Component>
        const Component* data() const { return (Component*) components_.data(); }

        template<typename Component>
        void assure(const std::size_t page) {
            if (page >= indirect_.size()) {
                if (indirect_.size() == 0) {
                    callDestructor_ = [](void* addr) { ((Component*) addr)->~Component(); };
                    componentSize_ = sizeof(Component);
                }

                indirect_.resize(page + 1, nullptr);
            }

            if (!indirect_[page]) {
                indirect_[page] = new entity_index_type[ENTITIES_PER_PAGE]{null};
            }
        }

        auto poolIndex(const Entity e) const {
            const auto index = e.index();
            const auto page = index / ENTITIES_PER_PAGE;
            const auto offset = index % ENTITIES_PER_PAGE;
            return std::make_pair(page, offset);
        }

        template<typename Component, typename... Args>
        Component* create(const Entity e, Args&&... args) {
            auto [page, offset] = poolIndex(e);
            assure<Component>(page);
            const auto pos = direct_.size();
            indirect_[page][offset] = entity_index_type(pos);
            direct_.push_back(e);
            components_.resize(direct_.size() * sizeof(Component));
            return new(&components_[pos * sizeof(Component)]) Component(std::forward<Args>(args)...);
        }

        /**
         * If the entity has the component, destroy it, then copy the last component into its
         * spot to keep the array packed tightly.
         */
        void destroy(const Entity e) {
            const auto [dPage, dOffset] = poolIndex(e);
            if (indirect_[dPage][dOffset] == null)
                return;

            const auto [toPage, toOffset] = poolIndex(direct_.back());

            const auto idx = indirect_[dPage][dOffset];
            void* dst = &components_[idx * componentSize_];
            callDestructor_(dst);
            void* src = &components_[(direct_.size() - 1) * componentSize_];
            memcpy(dst, src, componentSize_);
            components_.resize(components_.size() - componentSize_);

            direct_[indirect_[dPage][dOffset]] = direct_.back();
            direct_.pop_back();

            indirect_[toPage][toOffset] = indirect_[dPage][dOffset];
            indirect_[dPage][dOffset] = null;
        }

        bool has(const Entity e) {
            const auto [page, offset] = poolIndex(e);
            return page < indirect_.size() && indirect_[page][offset] != null;
        }

        template<typename Component>
        Component* get(const Entity e) {
            const auto [page, offset] = poolIndex(e);
            const auto idx = indirect_[page][offset];
            return (Component*) &components_[idx];
        }

    // private:
        std::vector<entity_index_type*> indirect_;
        std::vector<Entity> direct_;
        std::vector<uint8_t> components_;

        // When an entity is destroyed, all of its components should be destroyed,
        // but the types arent known at that point. As a result, need to save the Component size
        // and a function that properly calls the destructor when the Component type is
        // known (create method)
        std::function<void(void*)> callDestructor_;
        size_t componentSize_ = -1;
};


    namespace {

        EntityManager entityManager_;
        std::vector<ComponentPool> components_;

        template <typename Component>
        ComponentPool& assurePool() {
            const uint32_t cTypeID = Family<Component>::id();
            PG_ASSERT(cTypeID < MAX_COMPONENTS);
            return components_[cTypeID];
         }

    } // namespace anonymous

    // reserve space for 1K entities and initialize all comonent pools
    // Also create space for 64 different component types
    inline void init() {
        components_.resize(MAX_COMPONENTS);
    }

    inline void shutdown() {
        entityManager_.clear();
        components_.clear();
    }

    namespace entity {

        inline EntityData* data(const entity_type& id) {
            return entityManager_.getData(id);
        }

        inline Entity create() {
            return entityManager_.create();
        }

        inline bool alive(const Entity e) {
            return entityManager_.alive(e);
        }

        inline void destroy(const Entity e) {
            return entityManager_.destroy(e);
        }

    } // namespace entity

    namespace component {

        template <typename Component, typename... Args>
        Component* create(const Entity& e, Args&&... args) {
            auto& pool = assurePool<Component>();
            return pool.template create<Component, Args...>(e, std::forward<Args>(args)...);
        }

        template <typename Component>
        bool has(const Entity& e) {
            auto& pool = assurePool<Component>();
            return pool.has(e);
        }

        inline void destroy(uint32_t componentFamilyID, const entity_type& e) {
            PG_ASSERT(componentFamilyID < MAX_COMPONENTS);
            components_[componentFamilyID].destroy(e);
        }

        template <typename Component>
        void destroy(const Entity& e) {
            destroy(Family<Component>::id(), e);
        }

        template <typename Component>
        ComponentPool& getPool() {
            return assurePool<Component>();
        }

        template <typename Component, typename F>
        void for_each(F&& func) {
            auto& pool = assurePool<Component>();
            auto begin = pool.template begin<Component>();
            auto end = pool.template end<Component>();
            for (auto it = begin; it != end; ++it) {
                func(*it);
            }
        }

    } // namespace component

} // namespace ECS

} // namespace Progression