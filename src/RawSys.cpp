#include <iostream>
#include <string>
#include <tuple>
#include <fstream>
#include <array>
#include <vector>
#include <regex>
#include <filesystem>
#include <type_traits>
#include "Utils.h"

template<typename... Args>
struct Typelist {};

template<typename... Args>
struct ContainerInner
{
    std::tuple<std::vector<Args>...> m_elements;

    template<typename T>
    static constexpr bool containsOne()
    {
        return (std::is_same_v<T, Args> || ...);
    }

    template<typename... T>
    static constexpr bool contains()
    {
        return (containsOne<T>() && ...);
    }

    template<typename... T>
    static constexpr bool contains(Typelist<T...>)
    {
        return contains<T...>();
    }

    template<typename T>
    constexpr inline std::vector<T> &get()
    {
        return std::get<std::vector<T>>(m_elements);
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
        return std::get<ContainerInner<TT...>&>(m_tpl);
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


template<typename... T>
struct ArgList {
    template<typename... TTS>
    using add = ArgList<T..., ContainerInner<TTS...>>;
};


template<typename ListArgs>
struct Container;

template<typename... Args>
struct Container<ArgList<Args...>>
{
    std::tuple<Args...> m_containers;

    template<typename... T>
    constexpr inline auto getTuples()
    {
        return (getQueryElem<Typelist<T...>>(std::get<Args>(m_containers)) + ...);
    }

    template<typename... T>
    constexpr inline ContainerInner<T...> &get()
    {
        return std::get<ContainerInner<T...>>(m_containers);
    }
};

using MyArgList = ArgList<>
    ::add<char, int>
    ::add<char, float>
    ::add<char, int, float>;

int main(int argc, char* args[])
{    
    Container <MyArgList> cnt;

    auto arr = cnt.getTuples<int>();
    std::cout << typeid(arr).name() << std::endl;

    cnt.get<char, int>().get<int>().push_back(5);
    arr.get<char, int>().get<int>().push_back(-99);

    std::cout << cnt.get<char, int>().get<int>() << std::endl;
    std::cout << arr.get<char, int>().get<int>() << std::endl;
}
