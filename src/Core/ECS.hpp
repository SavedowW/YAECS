#include "Utils.h"
#include "TypeManip.hpp"
#include <tuple>
#include <concepts>
#include <vector>
#include <string>
#include <iostream>

using Entity = uint32_t;

template<typename T>
void printContainer(std::ostream &os_, const T &con_, int intend_)
{
    os_ << utils::getIntend(intend_) + utils::normalizeType(typeid(T).name()) << ": ";
    for (int i = 0; i < con_.size(); ++i)
        os_ << std::endl << utils::getIntend(intend_ + 4) << con_.at(i);
    os_ << std::endl;
}

template<typename T>
void eraseContainer(std::vector<T> &con_, Entity toErase_)
{
    if (con_.size() > 1 && con_.size() - 1 != toErase_)
        con_[toErase_] = std::move(con_.back());
    con_.pop_back();
}

template<typename... Args>
struct Archetype
{
    static_assert(sizeof...(Args) > 0, "Archetype should have at least 1 template parameter");
    static_assert(is_unique<Args...>, "All parameters in Archetype should be unique");

    std::tuple<std::vector<Args>...> m_components;

    void dump(int intend_)
    {
        std::cout << utils::getIntend(intend_) << utils::normalizeType(typeid(*this).name()) << std::endl;
        std::apply([intend_](auto&&... complst)
        {
            ((printContainer(std::cout, complst, intend_ + 4)), ...);
        }, m_components);
    }

    uint32_t addEntity(Args&&... args_)
    {
        auto pos = 0;
        ([&]
        {
            std::get<std::vector<Args>>(m_components).push_back(std::forward<Args>(args_));
            pos = std::get<std::vector<Args>>(m_components).size() - 1;
        } (), ...);

        return pos;
    }

    void removeEntity(Entity localEntityId_)
    {
        std::apply([&localEntityId_](auto&&... complst)
        {
            ((eraseContainer(complst, localEntityId_)), ...);
        }, m_components);
    }

    template<typename T>
    static constexpr bool containsOne()
    {
        return (std::is_same_v<T, Args> || ...);
    }

    template<typename... T>
    static constexpr bool contains()
    {
        static_assert(is_unique<T...>, "All parameters in Archetype::contains should be unique");
        return (containsOne<T>() && ...);
    }

    template<typename... T>
    static constexpr bool contains(Typelist<T...>)
    {
        static_assert(is_unique<T...>, "All parameters in Archetype::contains should be unique");
        return contains<T...>();
    }

    template<typename T>
    constexpr inline std::vector<T> &get()
    {
        return std::get<std::vector<T>>(m_components);
    }
};

template<typename CONTAINER_INNER, typename LST>
concept Contained = CONTAINER_INNER::template contains(LST());

template<typename... Args>
struct Query
{
    std::tuple<Args...> m_tpl;

    template<typename... TT>
    constexpr inline auto operator+(const Query<TT...> &rhs_)
    {
        return Query<Args..., TT...>(std::tuple_cat(m_tpl, rhs_.m_tpl));
    }

    template<typename... TT>
    constexpr inline auto &get()
    {
        return std::get<Archetype<TT...>&>(m_tpl);
    }
};

template<typename LST, typename CONTAINER_INNER>
constexpr inline auto getQueryElem(CONTAINER_INNER &t_)
{
    return Query<>();
}

template<typename LST, typename CONTAINER_INNER> requires Contained<CONTAINER_INNER, LST>
constexpr inline auto getQueryElem(CONTAINER_INNER &t_)
{
    return Query(std::tuple<CONTAINER_INNER&>(t_));
}

template<typename COMPONENT, typename ARCHETYPE>
constexpr inline auto &&getComponentConstructor(ARCHETYPE &arch_, int src_)
{
    return COMPONENT();
}

template<typename COMPONENT, typename ARCHETYPE> requires Contained<ARCHETYPE, Typelist<COMPONENT>>
constexpr inline auto &&getComponentConstructor(ARCHETYPE &arch_, int src_)
{
    return std::move(arch_.get<COMPONENT>()[src_]);
}

template<typename... T>
struct ArchList {
    template<typename... COMPS_T>
    using add = ArchList<T..., Archetype<COMPS_T...>>;
};

template<typename ListArches>
struct Registry;

template<typename... Args>
struct Registry<ArchList<Args...>>
{
    static_assert(is_unique<Args...>, "All parameters in Registry should be unique");
    static_assert(sizeof...(Args) > 0, "Registry should have at least 1 template parameter");

    std::tuple<Args...> m_archetypes;

    template<typename... COMP_T>
    Entity addEntity(COMP_T&&... components_)
    {
        return std::get<Archetype<COMP_T...>>(m_archetypes).addEntity(std::forward<COMP_T>(components_)...);
    }

    template<typename... COMP_T>
    void removeEntity(Entity localEntityId_)
    {
        std::get<Archetype<COMP_T...>>(m_archetypes).removeEntity(localEntityId_);
    }

    void dump(int intend_)
    {
        std::cout << utils::getIntend(intend_) + "" << utils::normalizeType(typeid(*this).name()) << std::endl;
        std::apply([&](auto&&... archt)
        {
            ((archt.dump(intend_ + 4)), ...);
        }, m_archetypes);
    }

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

    template<typename... T>
    constexpr inline auto getTuples()
    {
        static_assert(sizeof...(T) > 0, "Registry::getTuples should receive at least 1 template parameter");
        static_assert(is_unique<T...>, "All parameters in Registry::getTuples should be unique");
        return (getQueryElem<Typelist<T...>>(std::get<Args>(m_archetypes)) + ...);
    }

    template<typename... T>
    constexpr inline Archetype<T...> &get()
    {
        static_assert(sizeof...(T) > 0, "Registry::get should receive at least 1 template parameter");
        static_assert(is_unique<T...>, "All parameters in Registry::get should be unique");
        return std::get<Archetype<T...>>(m_archetypes);
    }

    template<typename ATYPE_SRC, typename ATYPE_DST>
    constexpr inline void convert(Entity entity_)
    {
        convert(std::get<ATYPE_SRC>(m_archetypes), std::get<ATYPE_DST>(m_archetypes), entity_);
    }

    template<typename ATYPE_SRC, typename... DST>
    constexpr inline void convert(ATYPE_SRC &src_, Archetype<DST...> &dst_, Entity entity_)
    {
        dst_.addEntity(std::move<DST>(getComponentConstructor<DST>(src_, entity_))...);
        src_.removeEntity(entity_);
    }

};

template<typename... T>
struct MakeQuery
{
template<typename... COMPS_T>
    using add = MakeQuery<T..., Archetype<COMPS_T...>&>;
    using build = Query<T...>;
};
