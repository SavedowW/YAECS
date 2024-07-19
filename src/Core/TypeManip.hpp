#include <concepts>

template<typename... TT>
concept TemplateExists = sizeof...(TT) > 0;

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
