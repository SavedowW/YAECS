#include <iostream>
#include <string>
#include "Utils.h"
#include "Collider.h"
#include <tuple>
#include <fstream>
#include "CoreComponents.h"
#include <array>
#include <vector>
#include <windows.h>
#include <Psapi.h>
#include <regex>

std::string getIntend(int intend_)
{
    return std::string(intend_, ' ');
}

std::string replaceAll(std::string src_, const std::string &replacable_, const std::string &toReplace_)
{
    return std::regex_replace(src_, std::regex(replacable_), toReplace_);
}

std::string wrap(const std::string &src_)
{
    return "\"" + src_ + "\"";
}

std::string normalizeType(const std::string &reg_)
{
    
    auto res = replaceAll(reg_, "struct ", "");
    res = replaceAll(res, "class ", "");
    res = replaceAll(res, " ", "");

    res = replaceAll(res, "([^ ]),([^ ])", "$1 , $2");
    res = replaceAll(res, " ,([^ ])", " , $1");
    res = replaceAll(res, "([^ ]), ", "$1 , ");

    res = replaceAll(res, "([^ ])<([^ ])", "$1 < $2");
    res = replaceAll(res, " <([^ ])", " < $1");
    res = replaceAll(res, "([^ ])< ", "$1 < ");

    res = replaceAll(res, "([^ ])>([^ ])", "$1 > $2");
    res = replaceAll(res, " >([^ ])", " > $1");
    res = replaceAll(res, "([^ ])> ", "$1 > ");

    return res;
}

constexpr inline size_t MAX_ENTITIES = 8000;

struct ComponentTransform
{
    ComponentTransform() = default;

    ComponentTransform(const Vector2<float> &pos_, const Vector2<float> &size_) :
        m_pos(pos_), m_size(size_)
    {
        //std::cout << __FUNCTION__ << " args\n";
    }

    ComponentTransform (const ComponentTransform &rhs_) = delete;

    ComponentTransform (ComponentTransform &&rhs_) :
        m_pos(rhs_.m_pos),
        m_size(rhs_.m_size)
    {
        //std::cout << __FUNCTION__ << " move\n";
    }

    ComponentTransform &operator=(const ComponentTransform &rhs_) = delete;

    ComponentTransform &operator=(ComponentTransform &&rhs_)
    {
        m_pos = rhs_.m_pos;
        m_size = rhs_.m_size;
        //std::cout << __FUNCTION__ << " move\n";
        return *this;
    }

    Vector2<float> m_pos;
    Vector2<float> m_size;
};

struct ComponentPhysical
{
    ComponentPhysical() = default;

    ComponentPhysical(const Collider &cld_, float gravity_) :
        m_cld(cld_), m_gravity(gravity_)
    {
        //std::cout << "ComponentPhysical CONSTRUCT ARGUMENTS\n";
    }

    ComponentPhysical (const ComponentPhysical &rhs_) = delete;

    ComponentPhysical (ComponentPhysical &&rhs_) :
        m_cld(rhs_.m_cld),
        m_gravity(rhs_.m_gravity)
    {
        //std::cout << "ComponentPhysical CONSTRUCT MOVE\n";
    }

    ComponentPhysical &operator=(const ComponentPhysical &rhs_) = delete;

    ComponentPhysical &operator=(ComponentPhysical &&rhs_)
    {
        m_cld = rhs_.m_cld;
        m_gravity = rhs_.m_gravity;
        //std::cout << "ComponentPhysical ASSIGN MOVE\n";
        return *this;
    }

    Collider m_cld;
    float m_gravity;
};

struct ComponentCharacter
{
    ComponentCharacter() = default;

    ComponentCharacter(const std::string &name_, int level_) :
        m_name(name_), m_level(level_)
    {
        //std::cout << "ComponentCharacter CONSTRUCT ARGUMENTS\n";
    }

    ComponentCharacter (const ComponentCharacter &rhs_) = delete;

    ComponentCharacter (ComponentCharacter &&rhs_) :
        m_name(rhs_.m_name),
        m_level(rhs_.m_level)
    {
        //std::cout << "ComponentCharacter CONSTRUCT MOVE\n";
    }

    ComponentCharacter &operator=(const ComponentCharacter &rhs_) = delete;

    ComponentCharacter &operator=(ComponentCharacter &&rhs_)
    {
        m_name = rhs_.m_name;
        m_level = rhs_.m_level;
        //std::cout << "ComponentCharacter ASSIGN MOVE\n";
        return *this;
    }

    std::string m_name;
    int m_level;
};


std::ostream &operator<<(std::ostream &os_, const ComponentTransform &comp_)
{
    os_ << "Transform ( pos: {" << comp_.m_pos << "}, size: {" << comp_.m_size << "} )";
    return os_;
}

std::ostream &operator<<(std::ostream &os_, const ComponentPhysical &comp_)
{
    os_ << "Physical ( collider: " << comp_.m_cld << ", gravity: {" << comp_.m_gravity << "} )";
    return os_;
}

std::ostream &operator<<(std::ostream &os_, const ComponentCharacter &comp_)
{
    os_ << "Character ( name: " << comp_.m_name << ", level: {" << comp_.m_level << "} )";
    return os_;
}

using Entity = uint32_t;
template<typename... Args>
struct Typelist {
    static_assert(is_unique<T...>, "All parameters in Typelist should be unique");
};

template<typename T>
void printContainer(std::ostream &os_, const T &con_, int intend_)
{
    std::cout << getIntend(intend_) + normalizeType(typeid(T).name()) << ": ";
    for (int i = 0; i < con_.size(); ++i)
        std::cout << std::endl << getIntend(intend_ + 4) << con_.at(i);
    std::cout << std::endl;
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
        std::cout << getIntend(intend_) << normalizeType(typeid(*this).name()) << std::endl;
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
        return std::get<std::vector<T>>(m_elements);
    }
};

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
        std::cout << getIntend(intend_) + "" << normalizeType(typeid(*this).name()) << std::endl;
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
    constexpr inline auto getArrs() const
    {
        static_assert(sizeof...(T) > 0, "Registry::getArrs should receive at least 1 template parameter");
        static_assert(is_unique<T...>, "All parameters in Registry::getArrs should be unique");

        std::array<int, count<T...>()> arr;
        return arr;
    }
};

using MyReg = ArchList<>
    ::add<ComponentTransform>
    ::add<ComponentTransform, ComponentPhysical>
    ::add<ComponentTransform, ComponentPhysical, ComponentCharacter>
    ::add<ComponentTransform, ComponentCharacter>;

struct TypeHash
{
    static int lastHash;
    
    template<typename T> 
    static int getHash()
    {
        static Hash = 0;
        return Hash++;
    }
};

int TypeHash::lastHash = 0;

int main(int argc, char* args[])
{    
    Registry<MyReg> reg;
    std::cout << reg.addEntity(ComponentTransform{{2.3f, -99.5f}, {1.0f, 2.0f}}, ComponentPhysical{{2.3f, 39.9f, 10.0f, 15.0f}, 9.8f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{1.1f, 1.2f}, {1.1f, 1.2f}}, ComponentPhysical{{1.1f, 1.2f, 1.3f, 1.4f}, 9.9f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{2.1f, 2.2f}, {2.1f, 2.2f}}, ComponentPhysical{{2.1f, 2.2f, 2.3f, 2.4f}, 10.0f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{3.1f, 3.2f}, {3.1f, 3.2f}}, ComponentPhysical{{3.1f, 3.2f, 3.3f, 3.4f}, 11.0f}) << std::endl;

    reg.removeEntity<ComponentTransform, ComponentPhysical>(1);

    std::cout << reg.addEntity(ComponentTransform{{4.1f, 4.2f}, {4.1f, 4.2f}}, ComponentPhysical{{4.1f, 4.2f, 4.3f, 4.4f}, 12.0f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-99.99f, -99.99f}, {1.0f, 2.0f}}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-100.99f, -100.99f}, {1.0f, 2.0f}}) << std::endl;

    std::cout << reg.addEntity(ComponentTransform{{-100.99f, -100.99f}, {1.0f, 2.0f}}, ComponentCharacter{"Nameless", 5}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-100.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{3.1f, 3.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless2", 7}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-100.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{4.1f, 4.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless3", 1}) << std::endl;
    reg.dump(0);

    PROCESS_MEMORY_COUNTERS memCounter;
    BOOL result = K32GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
    std::cout << "WorkingSetSize " << memCounter.WorkingSetSize / 1024 << std::endl;

    std::cout << reg.count<ComponentTransform, ComponentPhysical>() << std::endl;

    auto arr = reg.getArrs<ComponentTransform, ComponentPhysical>();

    std::cout << typeid(arr).name() << std::endl;


    std::cout << TypeHash::getHash<int>() << std::endl;
    std::cout << TypeHash::getHash<std::string>() << std::endl;
    std::cout << TypeHash::getHash<double>() << std::endl;
    std::cout << TypeHash::getHash<char>() << std::endl;
    std::cout << TypeHash::getHash<int>() << std::endl;
    std::cout << TypeHash::getHash<double>() << std::endl;
    std::cout << TypeHash::getHash<std::string>() << std::endl;
}
