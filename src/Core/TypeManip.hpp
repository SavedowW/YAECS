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

inline void Typelist<>::dump(std::ostream &os_)
{
    os_ << "(EMPTY)\n";
}


template <typename T>
struct return_type;

template <typename R, typename... Args>
struct return_type<R(Args...)> { using type = R; };

template <typename R, typename... Args>
struct return_type<R(*)(Args...)> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...)> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) &> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) &&> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) const> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) const&> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) const&&> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) volatile> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) volatile&> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) volatile&&> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) const volatile> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) const volatile&> { using type = R; };

template <typename R, typename C, typename... Args>
struct return_type<R(C::*)(Args...) const volatile&&> { using type = R; };

template <typename T>
using return_type_t = typename return_type<T>::type;


template <typename T, typename Tuple>
struct has_type;

template <typename T, typename... Us>
struct has_type<T, std::tuple<Us...>> : std::disjunction<std::is_same<T, Us>...> {};
