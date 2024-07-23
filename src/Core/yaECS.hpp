#ifndef YAECS_H_
#define YAECS_H_
#include "Utils.h"
#include "TypeManip.hpp"
#include <tuple>
#include <concepts>
#include <vector>
#include <string>
#include <iostream>

template<typename, typename>
class StateMachine;

namespace ECS
{
    // The system only indexes entities withing archetypes so its rarely used
    using Entity = uint32_t;

    template<typename... Components_t>
    using EntityRef = std::tuple<Components_t&...>;

    // Utility for debugging
    template<typename T>
    constexpr inline void printContainer(std::ostream &os_, const T &con_, int intend_)
    {
        os_ << utils::getIntend(intend_) + utils::normalizeType(typeid(T).name()) << ": ";
        for (int i = 0; i < con_.size(); ++i)
            os_ << std::endl << utils::getIntend(intend_ + 4) << con_.at(i);
        os_ << std::endl;
    }

    // Utility to erase element from vector by moving last element to it
    template<typename T>
    constexpr inline void eraseContainer(std::vector<T> &con_, Entity toErase_)
    {
        if (con_.size() > 1 && con_.size() - 1 != toErase_)
            con_[toErase_] = std::move(con_.back());
        con_.pop_back();
    }

    /*
        Archetype class
        Contains vectors of components in a tuple
        Entities are only ever indexed by indexes within these vectors
        Optimally, queries are used to iterate between them, as they cost nothing to create
        and provide compile-time component checks and very fast iteration
    */
    template<typename... Args> 
    class Archetype
    {

        static_assert(sizeof...(Args) > 0, "Archetype should have at least 1 template parameter");
        static_assert(is_unique<Args...>, "All parameters in Archetype should be unique");

    public:

        // Dump content to std::cout
        constexpr inline void dump(int intend_) const
        {
            std::cout << utils::getIntend(intend_) << utils::normalizeType(typeid(*this).name()) << std::endl;
            std::apply([intend_](auto&&... complst)
            {
                ((printContainer(std::cout, complst, intend_ + 4)), ...);
            }, m_components);
        }

        // Adds entity to the first free spot
        constexpr inline uint32_t addEntity(Args&&... args_)
        {
            auto pos = 0;
            ([&]
            {
                std::get<std::vector<Args>>(m_components).push_back(std::forward<Args>(args_));
                pos = std::get<std::vector<Args>>(m_components).size() - 1;
            } (), ...);

            return pos;
        }

        // Removes entity from archetype by moving last entity into its place
        constexpr inline void removeEntity(Entity localEntityId_)
        {
            std::apply([&localEntityId_](auto&&... complst)
            {
                ((eraseContainer(complst, localEntityId_)), ...);
            }, m_components);
        }

        // Checks if an archetype contains a component
        template<typename T>
        static constexpr bool containsOne()
        {
            return (std::is_same_v<T, Args> || ...);
        }

        // Checks if an archetype contains a number of components (in any order)
        template<typename... T>
        static constexpr bool contains()
        {
            static_assert(is_unique<T...>, "All parameters in Archetype::contains should be unique");
            return (containsOne<T>() && ...);
        }

        // Checks if an archetype contains a number of components, specified by passed typelist
        template<typename... T>
        static constexpr bool contains(TypeManip::Typelist<T...>)
        {
            static_assert(is_unique<T...>, "All parameters in Archetype::contains should be unique");
            return contains<T...>();
        }

        // Get component array for iteration, indexing, etc
        template<typename T>
        constexpr inline std::vector<T> &get()
        {
            return std::get<std::vector<T>>(m_components);
        }

        inline EntityRef<Args...> getEntity(Entity ent_)
        {
            return std::tuple<Args&...>((std::get<std::vector<Args>>(m_components)[ent_]) ...);
        }

        template<typename... Comps> requires TypeManip::TemplateExists<Comps...>
        inline EntityRef<Comps...> getEntity(Entity ent_)
        {
            return std::tuple<Comps&...>((std::get<std::vector<Comps>>(m_components)[ent_]) ...);
        }

        inline size_t size() const
        {
            return std::get<0>(m_components).size();
        }

    private:
        std::tuple<std::vector<Args>...> m_components;
    };

    template <typename... Ts, typename... Us>
    auto filter(Archetype<Us...>& c)
    {
        if constexpr (Archetype<Us...>::template contains<Ts...>()) {
            return std::tie(c);
        } else {
            return std::tuple<>{};
        }
    }

    // Checks that archetype contains components specified by Typelist
    template<typename ATYPE, typename LST>
    concept Contained = ATYPE::template contains(LST());

    /*
        A wrapper for tuple of references to archetypes
        Query is created by Registry to provide access to specified componenets
        As its type is deduced at compile time and it only contains references, making and accessing Queries and iterating through them are extremely fast operations
    */
    template<typename... Args>
    struct Query
    {
        std::tuple<Args...> m_tpl;

        // Concat Queries, used for unfolding
        template<typename... TT>
        constexpr inline auto operator+(const Query<TT...> &rhs_) const
        {
            return Query<Args..., TT...>(std::tuple_cat(m_tpl, rhs_.m_tpl));
        }

        // Get Archetype from Query
        template<typename... TT>
        constexpr inline auto &get()
        {
            return std::get<Archetype<TT...>&>(m_tpl);
        }
    };

    // Utility functions to either move a component from archetype or get an empty component, used to convery entities
    template<typename COMPONENT, typename ARCHETYPE>
    constexpr inline auto &&getComponentConstructor(ARCHETYPE &arch_, int src_)
    {
        return COMPONENT();
    }

    template<typename COMPONENT, typename ARCHETYPE> requires Contained<ARCHETYPE, TypeManip::Typelist<COMPONENT>>
    constexpr inline auto &&getComponentConstructor(ARCHETYPE &arch_, int src_)
    {
        return std::move(arch_.template get<COMPONENT>()[src_]);
    }

    // Utility to easily build archetype list
    template<typename... T>
    struct ArchList {
        template<typename... COMPS_T>
        using add = ArchList<T..., Archetype<COMPS_T...>>;

        template<typename... Ts>
        static inline constexpr auto AddTypelist(TypeManip::Typelist<Ts...>)
        {
            return add<Ts...>();
        }

        template<typename CompList> requires TypeManip::IsSpecialization<TypeManip::Typelist, CompList>
        using addTypelist = decltype(AddTypelist<>(CompList()));
    };

    /*
        Registry class
        Contains all archetypes, delegates add, delete operations and manages convertion
        Also provides Queries by request
    */
    template<typename ListArches>
    class Registry;

    template<typename... Args>
    class Registry<ArchList<Args...>>
    {
        static_assert(is_unique<Args...>, "All parameters in Registry should be unique");
        static_assert(sizeof...(Args) > 0, "Registry should have at least 1 template parameter");

    public:

        // Delegates to respective archetype
        template<typename... COMP_T>
        constexpr inline Entity addEntity(COMP_T&&... components_)
        {
            return std::get<Archetype<COMP_T...>>(m_archetypes).addEntity(std::forward<COMP_T>(components_)...);
        }

        // Delegates to respective archetype
        template<typename... COMP_T>
        constexpr inline void removeEntity(Entity localEntityId_)
        {
            std::get<Archetype<COMP_T...>>(m_archetypes).removeEntity(localEntityId_);
        }

        // Dump content into std::cout
        void dump(int intend_) const
        {
            std::cout << utils::getIntend(intend_) + "" << utils::normalizeType(typeid(*this).name()) << std::endl;
            std::apply([&](auto&&... archt)
            {
                ((archt.dump(intend_ + 4)), ...);
            }, m_archetypes);
        }

        // Count archetypes that contain a list of components in any order
        template<typename... T>
        static constexpr size_t count()
        {
            static_assert(sizeof...(T) > 0, "Registry::count<> should receive at least 1 template parameter");

            size_t res = 0;

            ([&]
            {
                if (Args::template contains<T...>())
                    res++;

            } (), ...);

            return res;
        }

        /*
            Request query with all archetypes that contain all specified components
            If you want something other that conjunction, feel free to call this as much as you want, this operation really fast
            Might potentially extend to be able to query archetypes as < INCLUDE<COMPONENT1, COMPONENT2>, EXCLUDE<COMPONENT1, COMPONENT2>>
        */
        template<typename... T>
        constexpr inline auto getQuery()   
        {
            static_assert(sizeof...(T) > 0, "Registry::getTuples should receive at least 1 template parameter");
            static_assert(is_unique<T...>, "All parameters in Registry::getTuples should be unique");
            return Query{std::tuple_cat(filter<T...>(std::get<Args>(m_archetypes))...)};
        }

        /*
            Request query with all archetypes that contain all specified components in passed typelist
            If you want something other that conjunction, feel free to call this as much as you want, this operation really fast
            Might potentially extend to be able to query archetypes as < INCLUDE<COMPONENT1, COMPONENT2>, EXCLUDE<COMPONENT1, COMPONENT2>>
        */
        template<typename... T>
        constexpr inline auto getQueryTl(const TypeManip::Typelist<T...> &tl)
        {
            static_assert(sizeof...(T) > 0, "Registry::getTuples should receive at least 1 template parameter");
            static_assert(is_unique<T...>, "All parameters in Registry::getTuples should be unique");
            return Query{std::tuple_cat(filter<T...>(std::get<Args>(m_archetypes))...)};
        }


        /*
            Get archetype with specified components in this specific order
        */
        template<typename... T>
        constexpr inline Archetype<T...> &get()
        {
            static_assert(sizeof...(T) > 0, "Registry::get should receive at least 1 template parameter");
            static_assert(is_unique<T...>, "All parameters in Registry::get should be unique");
            return std::get<Archetype<T...>>(m_archetypes);
        }

        /*
            Converts entity from specified archetype to another
        */
        template<typename ATYPE_SRC, typename ATYPE_DST>
        constexpr inline void convert(Entity entity_)
        {
            convert(std::get<ATYPE_SRC>(m_archetypes), std::get<ATYPE_DST>(m_archetypes), entity_);
        }

        /*
            Converts entity from specified archetype to another, only used internally to split archetype into variadic template
        */
        template<typename ATYPE_SRC, typename... DST>
        constexpr inline void convert(ATYPE_SRC &src_, Archetype<DST...> &dst_, Entity entity_)
        {
            dst_.addEntity(std::move<DST>(getComponentConstructor<DST>(src_, entity_))...);
            src_.removeEntity(entity_);
        }

    private:
        std::tuple<Args...> m_archetypes;

    };

    template<typename... CompList>
    struct EntityData
    {
        using ComponentList = TypeManip::Typelist<CompList...>;
        using MakeRef = EntityRef<CompList...>;

        template<typename STATES_T>
        using WithSM = TypeManip::Typelist<CompList..., StateMachine<MakeRef, STATES_T>>;
    };

}

template<typename... Components_t>
std::ostream &operator<<(std::ostream &os_, const std::tuple<Components_t...> &rhs_)
{
    auto sz = sizeof...(Components_t);
    ((os_ << std::get<Components_t>(rhs_) << (--sz > 0 ? ", " : "")), ...);
    return os_;
}

#endif
