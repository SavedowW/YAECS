#include <concepts>
#include <functional>
#include <type_traits>

// Checks that variadic template is not empty
template<typename... TT>
concept TemplateExists = sizeof...(TT) > 0;

// Container that only contains list of types in its type definition
template<typename... Args>
struct Typelist {
    static_assert(is_unique<Args...>, "All parameters in Typelist should be unique");

    template<typename... TT>
    auto operator+(const Typelist<TT...> &rhs_)
    {
        return Typelist<Args..., TT...>();
    }

    static inline void dump(std::ostream &os_);
};

/*
    Structure that assigns hashes to every type its methods were called in this exact order
    Not actually used in this project, but can be very useful in some cases
*/
struct TypeHash
{
    static int LastHash;
    
    template<typename T> 
    static constexpr inline int getHash()
    {
        static int Hash = LastHash++;
        return Hash;
    }

    template<typename T, typename... TT> requires TemplateExists<TT...>
    static constexpr inline int getHash()
    {
        return getHash<T>();
    }
};

int TypeHash::LastHash = 0;

// Utility functions to dump type data

template<typename T>
constexpr inline void printTypesRecursive(std::ostream &os_)
{
    os_ << typeid(T).name() << " (" << TypeHash::getHash<T>() << ")\n";
}

template<typename T, typename... Rest> requires TemplateExists<Rest...>
constexpr inline void printTypesRecursive(std::ostream &os_)
{
    printTypesRecursive<T>(os_);
    printTypesRecursive<Rest...>(os_);
}

template <typename... Args>
inline void Typelist<Args...>::dump(std::ostream &os_)
{
    printTypesRecursive<Args...>(os_);
}

template<>
inline void Typelist<>::dump(std::ostream &os_)
{
    os_ << "(EMPTY)\n";
}
