#ifndef TYPE_MANIP_H_
#define TYPE_MANIP_H_
#include <concepts>
#include <functional>
#include <type_traits>

namespace TypeManip
{
    // Checks that variadic template is not empty
    template<typename... TT>
    concept TemplateExists = sizeof...(TT) > 0;


    template<template<typename...> typename A, template<typename...> typename B, typename... Params_t>
    constexpr inline bool isSpecializationF(B<Params_t...>*)
    {
        return std::is_same_v<A<Params_t...>, B<Params_t...>>;
    }

    // Checks that Spec_t is specialization of template Base_t
    template<template<typename...> typename Base_t, typename Spec_t>
    concept IsSpecialization = isSpecializationF<Base_t>(static_cast<Spec_t*>(nullptr));


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

        using ToEntityRef = std::tuple<Args&...>;
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

    template<typename T, int ID>
    struct MapField
    {
        using Type = T;
        static constexpr inline int Value = ID;
    };
    
    template<typename T, typename Current, typename... Rest>
    constexpr int GetRecursive()
    {
        if constexpr (std::is_same_v<T, typename Current::Type>)
        {
            return Current::Value;
        }
        else
        {
            static_assert(sizeof...(Rest) > 0, "Type was not found");
            return GetRecursive<T, Rest...>();
        }
    }
    
    template<int Next = 0, typename... Ts>
    struct TypeRegistry
    {
        using Create = TypeRegistry<0>;
    
        template<typename T>
        using Add = TypeRegistry<Next + 1, Ts..., MapField<T, Next>>;
    
        template<typename T>
        constexpr static int Get()
        {
            return GetRecursive<T, Ts...>();
        }
    };
    
    template<typename TypeRegistry, typename T>
    constexpr bool isSorted()
    {
        return true;
    }
    
    template<typename TypeRegistry, typename T1, typename T2, typename ...Rest>
    constexpr bool isSorted()
    {
        if constexpr(TypeRegistry::template Get<T1>() <= TypeRegistry::template  Get<T2>())
        {
            return isSorted<T2, Rest...>();
        }
        else
        {
            return false;
        }
    }
    
    template<typename TypeRegistry, typename... Ts>
    constexpr bool isSorted(TypeManip::Typelist<Ts...>)
    {
        return isSorted<TypeRegistry, Ts...>();
    }
    
    template<typename TypeRegistry, typename T>
    constexpr auto pushForward(TypeManip::Typelist<T>)
    {
        return TypeManip::Typelist<T>();
    }
    
    template<typename TypeRegistry, typename T1, typename T2, typename... Rest>
    constexpr auto pushForward(TypeManip::Typelist<T1, T2, Rest...>)
    {
        if constexpr (TypeRegistry::template Get<T1>() <= TypeRegistry::template  Get<T2>())
            return TypeManip::Typelist<T1, T2, Rest...>();
        else
            return TypeManip::Typelist<T2>() + pushForward<TypeRegistry>(TypeManip::Typelist<T1, Rest...>());
    }
    
    template<typename TypeRegistry, typename T>
    constexpr auto sort(TypeManip::Typelist<T>)
    {
        return TypeManip::Typelist<T>();
    }
    
    template<typename TypeRegistry, typename T1, typename T2, typename... Rest>
    constexpr auto sort(TypeManip::Typelist<T1, T2, Rest...>)
    {
    
        return pushForward<TypeRegistry>(TypeManip::Typelist<T1>() + sort<TypeRegistry>(TypeManip::Typelist<T2, Rest...>()));
    }

    template<template<typename...> typename Base_t, typename T>
    constexpr bool isLastSpecialization()
    {
        return IsSpecialization<Base_t, T>;
    }

    template<template<typename...> typename Base_t, typename T, typename... Rest> requires TemplateExists<Rest...>
    constexpr bool isLastSpecialization()
    {
        return isLastSpecialization<Base_t, Rest...>();
    }

}

#endif
