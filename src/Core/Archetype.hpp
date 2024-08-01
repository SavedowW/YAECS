#ifndef ARCHETYPE_H_
#define ARCHETYPE_H_
#include "UntypeContainer.h"
#include <bitset>
#include <iostream>

namespace ECS
{
    /*
        Dynamic view for an entity in archetype
        Only used for immediate access to its components, mostly needed for state machine
        Is outdated after most operations with even unrelated entities or components
    */
    class EntityView
    {
    public:
        template<typename TReg, typename T>
        T &get()
        {
            return *static_cast<T*>(m_components[TReg::template Get<T>()]);
        }

        template<typename T>
        T &get(std::size_t comp_)
        {
            return *static_cast<T*>(m_components[comp_]);
        }

        template<typename TReg, typename T>
        bool contains() const
        {
            return m_components.contains(TReg::template Get<T>());
        }

        void add(int id_, void* comp_)
        {
            m_components[id_] = comp_;
        }

    private:
        std::unordered_map<int, void*> m_components;
    };

    /*
        Archetype class
        A container for a number of component containers
        These components should be defined once right after initialization
        Its technically possible to add new components, but it most likely will have wrong capacity and size
    */
    template<typename TReg>
    class Archetype
    {
    public:
        Archetype() = default;

        Archetype(const Archetype<TReg> &rhs_) = delete;
        Archetype &operator=(const Archetype<TReg> &rhs_) = delete;

        Archetype(Archetype<TReg> &&rhs_) :
            m_map(std::move(rhs_.m_map))
        {
        }

        Archetype &operator=(Archetype<TReg> &&rhs_)
        {
            m_map = std::move(rhs_.m_map);
        }

        // Reserves all specified components if they werent reserved already
        template<typename... Ts> requires TypeManip::TemplateExists<Ts...>
        void addTypes(int reserve_)
        {
            (addType<Ts>(reserve_), ...);
        }

        // Reserves all specified components and components in another Archetype if they werent reserved already
        template<typename... Ts> requires TypeManip::TemplateExists<Ts...>
        void addTypes(const Archetype<TReg> &copied_, int reserve_)
        {
            iterateAddTypes<1, Ts...>(copied_, reserve_);
        }

        // Reserves all components from another archetype except listed ones
        template<typename... Ts> requires TypeManip::TemplateExists<Ts...>
        void addTypesReduced(const Archetype<TReg> &copied_, int reserve_)
        {
            iterateAddTypesExceptListed<1, Ts...>(copied_, reserve_);
        }


        template<typename... Ts> requires TypeManip::TemplateExists<Ts...>
        bool containsComponents() const
        {
            return (m_map.contains(TReg::template Get<Ts>()) && ...);
        }

        bool containsComponent(int comp_) const
        {
            return (m_map.contains(comp_));
        }

        inline size_t size() const
        {
            return m_map.begin()->second.size();
        }

        template<typename... Ts> requires TypeManip::TemplateExists<Ts...>
        void addEntity(Ts&&... ts_)
        {
            size_t cnt = 0;
            ([&]
            {
                if (m_map.contains(TReg::template Get<Ts>()))
                {
                    m_map[TReg::template Get<Ts>()].push_back(std::forward<Ts>(ts_));
                    cnt++;
                }
            } (), ...);

            if (cnt < m_map.size())
                throw std::exception();
        }

        size_t addEntity()
        {
            for (auto &el : m_map)
            {
                el.second.push_back();
            }
            return size() - 1;
        }

        template<typename... Ts>
        void emplaceComponents(std::size_t id_, Ts&&... comps_)
        {
            ([&]
            {
                if (m_map.contains(TReg::template Get<Ts>()))
                {
                    m_map[TReg::template Get<Ts>()].emplace(std::forward<Ts>(comps_), id_);
                }
                else
                {
                    throw std::exception();
                }
            } (), ...);
        }

        inline void removeEntity(size_t entity_)
        {
            for (auto &el : m_map)
            {
                el.second.removeAt(entity_);
            }
        }

        void dumpAll()
        {
            std::cout << " - Archetype " << getMask() << " - " << std::endl;
            auto sz = size();
            for (int i = 0; i < sz; ++i)
                dumpEntity(i);
        }

        void dumpEntity(size_t ent_)
        {
            dumpEntityComponents<1, TReg::MaxID>(ent_);

            std::cout << std::endl;
        }

        template<typename Comp>
        Comp &getComponent(std::size_t ent_)
        {
            return m_map[TReg::template Get<Comp>()].template get<Comp>(ent_);
        }

        template<typename... Comps>
        std::tuple<Comps&...> makeView(size_t ent_)
        {
            return std::tie<Comps&...>(m_map.at(TReg::template Get<Comps>()).template get<Comps>(ent_)...);
        }
        
        std::bitset<TReg::MaxID> getMask() const
        {
            std::bitset<TReg::MaxID> bset(0);
            for (auto &el : m_map)
                bset[el.first - 1] = 1;
            
            return bset;
        }

    private:
        std::unordered_map<int, UntypeContainer> m_map;

        template<typename T>
        void addType(int reserve_)
        {
            if (!m_map.contains(TReg::template Get<T>()))
                m_map[TReg::template Get<T>()].template allocate<T>(reserve_);
        }

        template<int CurrentType, typename... Ts>
        void iterateAddTypes(const Archetype<TReg> &copied_, int reserve_)
        {
            std::cout << CurrentType << std::endl;
            if constexpr (TypeManip::isListed<typename TReg::template GetById<CurrentType>, Ts...>())
            {
                addType<typename TReg::template GetById<CurrentType>>(reserve_);
            }
            else
            {
                if (copied_.containsComponents<typename TReg::template GetById<CurrentType>>())
                    addType<typename TReg::template GetById<CurrentType>>(reserve_);
            }

            if constexpr (CurrentType < TReg::MaxID)
                iterateAddTypes<CurrentType + 1, Ts...>(copied_, reserve_);
        }

        template<int CurrentType, typename... Ts>
        void iterateAddTypesExceptListed(const Archetype<TReg> &copied_, int reserve_)
        {
            if constexpr (!TypeManip::isListed<typename TReg::template GetById<CurrentType>, Ts...>())
            {
                if (copied_.containsComponents<typename TReg::template GetById<CurrentType>>())
                    addType<typename TReg::template GetById<CurrentType>>(reserve_);
            }

            if constexpr (CurrentType < TReg::MaxID)
                iterateAddTypesExceptListed<CurrentType + 1, Ts...>(copied_, reserve_);
        }

        template<int current, int max>
        void dumpEntityComponents(size_t ent_)
        {
            if (m_map.contains(current))
            {
                std::cout << m_map[current].template get<typename TReg::template GetById<current>>(ent_) << ", ";
            }

            if constexpr (current < max)
                dumpEntityComponents<current + 1, max>(ent_);
        }
    };
}

#endif