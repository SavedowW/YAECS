#ifndef YAECS_H_
#define YAECS_H_
#include "Utils.h"
#include "TypeManip.hpp"
#include "Archetype.hpp"
#include <tuple>
#include <concepts>
#include <vector>
#include <string>
#include <iostream>
#include <bitset>
#include <unordered_set>

namespace ECS
{
    template<typename TReg>
    class Registry;

    struct EntityIndex
    {
        size_t m_archetypeId;
        size_t m_entityId;
    };

    /*
        Another view for entity
    */
    template<typename TReg>
    class CheapEntityView
    {
    public:
        CheapEntityView(Registry<TReg> &reg_, const EntityIndex &idx_) :
            m_reg(reg_),
            m_idx(idx_)
        {
        }

        template<typename T>
        T &get()
        {
            return m_reg[m_idx.m_archetypeId].template getComponent<T>(m_idx.m_entityId);
        }

        template<typename... Ts>
        bool contains() const
        {
            return m_reg[m_idx.m_archetypeId].template  containsComponents<Ts...>();
        }

    private:
        Registry<TReg> &m_reg;
        const EntityIndex m_idx;
    };


    template<typename TReg>
    struct Query
    {
        Registry<TReg> &m_reg;
        std::vector<size_t> m_archIds;

        // Iterates forward, from first archetype to last, from first entity to last, passes head, index and required components
        template<typename... Comps, typename F, typename... Head> 
        void apply(F f_, Head&&... head_)
        {
            EntityIndex idx;
            for (size_t arch = 0; arch < m_archIds.size(); ++arch)
            {
                idx.m_archetypeId = m_archIds[arch];
                if (!m_reg[idx.m_archetypeId].containsComponents<Comps...>())
                    continue;

                for (idx.m_entityId = 0; idx.m_entityId < m_reg[idx.m_archetypeId].size(); ++idx.m_entityId)
                {
                    f_(std::forward<Head>(head_)..., idx, m_reg[idx.m_archetypeId].template getComponent<Comps>(idx.m_entityId)...);
                }
            }
        }

        /*
            Iterates backward, from last archetype to first, from last entity to first, passes head, index and required components
            Is guaranteed to work well with entity remove / add operations
            In case of remove, last entity will be pushed to the current position and wont be processed twice
            In case of add, new entity will be pushed to the end of the list and will not be proceded
        */
        template<typename... Comps, typename F>
        void revapply(F f_)
        {
            EntityIndex idx;
            for (size_t arch = m_archIds.size() - 1; true; --arch)
            {
                idx.m_archetypeId = m_archIds[arch];
                if (!m_reg[idx.m_archetypeId].containsComponents<Comps...>() || m_reg[idx.m_archetypeId].size() == 0)
                {
                    if (arch == 0)
                        break;
                    else
                        continue;
                }

                for (idx.m_entityId = m_reg[idx.m_archetypeId].size() - 1; true; --idx.m_entityId)
                {
                    f_(idx, m_reg[idx.m_archetypeId].template getComponent<Comps>(idx.m_entityId)...);

                    if (idx.m_entityId == 0)
                        break;
                }

                if (arch == 0)
                    break;
            }
        }

        // Iterates forward, from first archetype to last, from first entity to last, passes head, index and view
        template<typename F, typename... Head> 
        void applyview(F f_, Head&&... head_)
        {
            EntityIndex idx;
            for (size_t arch = 0; arch < m_archIds.size(); ++arch)
            {
                idx.m_archetypeId = m_archIds[arch];

                for (idx.m_entityId = 0; idx.m_entityId < m_reg[idx.m_archetypeId].size(); ++idx.m_entityId)
                {
                    f_(std::forward<Head>(head_)..., idx, CheapEntityView(m_reg, idx));
                }
            }
        }
    };

    template<typename TReg>
    class Registry
    {
    public:
        template<typename... Comps, typename... Emplaced> requires TypeManip::TemplateExists<Comps...>
        EntityIndex createEntity(Emplaced&&... comps_)
        {
            auto archid = getEnsureArchetype<Comps...>();
            EntityIndex newent {archid, m_archetypes[archid].addEntity()};
            m_archetypes[archid].emplaceComponents(newent.m_entityId, std::forward<Emplaced>(comps_)...);
            return newent;
        }

        // Components are expected to be unique
        template<typename... Comps> requires TypeManip::TemplateExists<Comps...>
        EntityIndex emplaceComponents(const EntityIndex &idx_, Comps&&... comps_)
        {
            if (m_archetypes[idx_.m_archetypeId].template containsComponents<Comps...>())
            {
                m_archetypes[idx_.m_archetypeId].emplaceComponents(idx_.m_entityId, std::forward<Comps>(comps_)...);
                return idx_;
            }
            else
            {
                auto oldmask = m_archetypes[idx_.m_archetypeId].getMask();
                auto newmask = extendMask<Comps...>(oldmask);

                auto archid = getEnsureCopiedArchetype<Comps...>(idx_.m_archetypeId, newmask);
                auto entId = m_archetypes[archid].addEntity();

                
                m_archetypes[archid].emplaceComponents(entId, std::forward<Comps>(comps_)...);
                recursiveEmplace<1, Comps...>(m_archetypes[idx_.m_archetypeId], m_archetypes[archid], idx_.m_entityId, entId);
                m_archetypes[idx_.m_archetypeId].removeEntity(idx_.m_entityId);

                return {archid, entId};
            }
        }

        // Components are expected to be unique
        template<typename... Comps> requires TypeManip::TemplateExists<Comps...>
        EntityIndex removeComponents(const EntityIndex &idx_)
        {
            auto newmask = removeFromMask<Comps...>(m_archetypes[idx_.m_archetypeId].getMask());
            auto newarch = 0;

            // Ensure that archetype with same components except listed exists
            auto fnd = m_archTypes.find(newmask);
            if (fnd == m_archTypes.end())
            {
                std::cout << "Archetype " << newmask << " doesn't exist, creating new\n";
                newarch = m_archetypes.size();
                m_archTypes[newmask] = newarch;
                m_archetypes.emplace_back();
                m_archetypes[newarch].addTypesReduced<Comps...>(m_archetypes[idx_.m_archetypeId], 5);
            }
            else
                newarch = fnd->second;

            auto newent = m_archetypes[newarch].addEntity();
            recursiveMoveAllRequired<1>(m_archetypes[idx_.m_archetypeId], m_archetypes[newarch], idx_.m_entityId, newent);
            m_archetypes[idx_.m_archetypeId].removeEntity(idx_.m_entityId);

            return idx_;
        }

        void removeEntity(const EntityIndex &idx_)
        {
            m_archetypes[idx_.m_archetypeId].removeEntity(idx_.m_entityId);
        }

        void dumpAll()
        {
            std::cout << "=== REGISTRY === " << std::endl;
            for (auto &el : m_archetypes)
                el.dumpAll();
        }

        template<typename... Comps>
        Query<TReg> makeQuery()
        {
            Query<TReg> res(*this);

            for (size_t archId = 0; archId < m_archetypes.size(); ++archId)
            {
                auto &arch = m_archetypes[archId];
                if (arch.containsComponents<Comps...>())
                    res.m_archIds.push_back(archId);
            }

            return res;
        }

        Archetype<TReg> &operator[](std::size_t rhs_)
        {
            return m_archetypes[rhs_];
        }

        template<typename T>
        T &getComponent(const EntityIndex &ent_)
        {
            return m_archetypes[ent_.m_archetypeId].getComponent<T>(ent_.m_entityId);
        }

    private:
        template<typename... Comps> requires TypeManip::TemplateExists<Comps...>
        size_t getEnsureArchetype()
        {
            constexpr std::bitset<TReg::MaxID> bset(((1ull << (TReg::template Get<Comps>() - 1)) | ...));

            auto fnd = m_archTypes.find(bset);

            if (fnd != m_archTypes.end())
            {
                std::cout << "Found archetype " << bset << ", creating entity there" << std::endl;
                return fnd->second;
            }
            else
            {
                std::cout << "Couldn't find archetype " << bset << ", creating new" << std::endl;
                auto newid = m_archetypes.size();
                m_archTypes[bset] = newid;
                m_archetypes.emplace_back();
                m_archetypes[newid].template addTypes<Comps...>(10);
                return newid;
            }
        }

        template<typename... Comps> requires TypeManip::TemplateExists<Comps...>
        size_t getEnsureCopiedArchetype(size_t oldArchId_, const std::bitset<TReg::MaxID> &newMask_)
        {
            auto fnd = m_archTypes.find(newMask_);

            if (fnd != m_archTypes.end())
            {
                std::cout << "Found archetype " << newMask_ << " by mask" << std::endl;
                return fnd->second;
            }
            else
            {
                std::cout << "Couldn't find archetype " << newMask_ << " by mask" << std::endl;
                std::size_t newid = m_archetypes.size();
                m_archTypes[newMask_] = newid;
                m_archetypes.emplace_back();
                m_archetypes[newid].template addTypes<Comps...>(m_archetypes.at(oldArchId_), 10);
                return newid;
            }
        }

        template<int CurrentType, typename... Emplaced>
        void recursiveEmplace(Archetype<TReg> &oldArch_, Archetype<TReg> &newArch_, std::size_t oldId_, std::size_t newId_)
        {
            if constexpr(!(TypeManip::template isListed<typename TReg::template GetById<CurrentType>, Emplaced...>()))
            {
                if (oldArch_.template containsComponents<typename TReg::template GetById<CurrentType>>())
                {
                    newArch_.template emplaceComponents<typename TReg::template GetById<CurrentType>>(newId_, std::move(oldArch_.template getComponent<typename TReg::template GetById<CurrentType>>(oldId_)));
                }
            }

            if constexpr (CurrentType < TReg::MaxID)
                recursiveEmplace<CurrentType + 1, Emplaced...>(oldArch_, newArch_, oldId_, newId_);
        }

        template<int CurrentType>
        void recursiveMoveAllRequired(Archetype<TReg> &oldArch_, Archetype<TReg> &newArch_, std::size_t oldId_, std::size_t newId_)
        {
            if (newArch_.containsComponents<typename TReg::template GetById<CurrentType>>())
            {
                newArch_.emplaceComponents(newId_, std::move(oldArch_.template getComponent<typename TReg::template GetById<CurrentType>>(oldId_)));
            }

            if constexpr (CurrentType < TReg::MaxID)
                recursiveMoveAllRequired<CurrentType + 1>(oldArch_, newArch_, oldId_, newId_);
        }

        template<typename... Ts>
        static constexpr std::bitset<TReg::MaxID> extendMask(const std::bitset<TReg::MaxID> &bitset_)
        {
            std::bitset<TReg::MaxID> bset2(((1ull << (TReg::template Get<Ts>() - 1)) | ...));
            return bitset_ | bset2;
        }

        template<typename... Ts>
        static constexpr std::bitset<TReg::MaxID> removeFromMask(const std::bitset<TReg::MaxID> &bitset_)
        {
            std::bitset<TReg::MaxID> bset2(((~(1ull << (TReg::template Get<Ts>() - 1))) & ...));
            return bitset_ & bset2;
        }

        std::vector<Archetype<TReg>> m_archetypes;
        std::unordered_map<std::bitset<TReg::MaxID>, size_t> m_archTypes;

    };
}

#endif
