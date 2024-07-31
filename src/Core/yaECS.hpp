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

template<typename, typename>
class StateMachine;

namespace ECS
{
    struct EntityIndex
    {
        size_t m_archetypeId;
        size_t m_entityId;
    };

    template<typename TReg>
    class Registry;

    template<typename TReg>
    struct Query
    {
        Registry<TReg> &m_reg;
        std::vector<size_t> m_archIds;
    };

    template<typename TReg>
    class Registry
    {
    public:
        template<typename... Comps> requires TypeManip::TemplateExists<Comps...>
        EntityIndex createEntity()
        {
            auto archid = getEnsureArchetype<Comps...>();
            return {archid, m_archetypes[archid].addEntity()};
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

            constexpr std::bitset<TReg::MaxID> bset(((1ull << (TReg::template Get<Comps>() - 1)) | ...));
            std::cout << "Quering by mask " << bset << std::endl;
            for (auto &el : m_archTypes)
            {
                if ((el.first & bset) == bset)
                {
                    std::cout << "Archetype " << el.first << " added\n";
                    res.m_archIds.push_back(el.second);
                }
                else
                    std::cout << "Archetype " << el.first << " failed\n";
            }

            return res;
        }

    // TODO:
    // Remove component from entity
    // Remove entity
    // Make cross-archetype view
    // Make iteration over view


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
