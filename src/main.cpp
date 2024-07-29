#include "Utils.h"
#include "TypeManip.hpp"
#include "ExampleComponents.h"
#include <iostream>
#include <unordered_map>
#include <exception>

using Components = TypeManip::TypeRegistry<>::Create
    ::Add<ComponentTransform>
    ::Add<ComponentPhysical>
    ::Add<ComponentCharacter>
    ::Add<ComponentPlayerInput>
    ::Add<ComponentMobNavigation>;

class UntypeContainer
{
public:
    template<typename T>
    UntypeContainer(int count_, T *ptr_) :
        m_entrySize(sizeof(T)),
        m_capacity(count_)
    {
        m_data = new T[count_];

        m_cleaner = [](void* data_)
        {
            delete []static_cast<T*>(data_);
        };
    }

    UntypeContainer() = default;

    UntypeContainer(const UntypeContainer &rhs_) = delete;
    UntypeContainer &operator=(const UntypeContainer &rhs_) = delete;

    UntypeContainer(UntypeContainer &&rhs_) :
        m_data(rhs_.m_data),
        m_capacity(rhs_.m_capacity),
        m_size(rhs_.m_size),
        m_entrySize(rhs_.m_entrySize),
        m_cleaner(rhs_.m_cleaner)
    {
        rhs_.m_data = nullptr;
        rhs_.m_capacity = 0;
        rhs_.m_size = 0;
        rhs_.m_entrySize = 0;
        rhs_.m_cleaner = nullptr;
    }
    
    UntypeContainer &operator=(UntypeContainer &&rhs_)
    {
        m_data = rhs_.m_data;
        m_capacity = rhs_.m_capacity;
        m_size = rhs_.m_size;
        m_entrySize = rhs_.m_entrySize;
        m_cleaner = rhs_.m_cleaner;

        rhs_.m_data = nullptr;
        rhs_.m_capacity = 0;
        rhs_.m_size = 0;
        rhs_.m_entrySize = 0;
        rhs_.m_cleaner = nullptr;

        return *this;
    }

    template<typename T>
    bool allocate(int count_)
    {
        if (m_data)
            return false;

        m_entrySize = sizeof(T);
        m_capacity = count_;
        m_size = 0;
        m_data = new T[count_];
        m_cleaner = [](void* data_)
        {
            delete []static_cast<T*>(data_);
        };

        return true;
    }

    template<typename T>
    bool allocated() const
    {
        return m_data;
    }

    template<typename T>
    inline bool freemem()
    {
        if (!m_data)
            return false;

        delete []static_cast<T*>(m_data);
        m_entrySize = 0;
        m_capacity = 0;
        m_data = nullptr;
        m_cleaner = nullptr;

        return true;
    }

    template<typename T>
    T &get(size_t id_)
    {
        return static_cast<T*>(m_data)[id_];
    }

    size_t size() const
    {
        return m_size;
    }

    template <typename T>
    void push_back(T &&rhs_)
    {
        if (m_size == m_capacity)
            realloc<T>(m_capacity * 1.3);

        static_cast<T*>(m_data)[m_size] = std::move(rhs_);
        ++m_size;
    }

    template<typename T>
    bool reposLastTo(size_t newIdx_)
    {
        if (newIdx_ >= m_size - 1)
            return false;

        auto *realarr = static_cast<T*>(m_data);
        --m_size;
        realarr[newIdx_] = std::move(realarr[m_size]);
        return true;
    }

    ~UntypeContainer()
    {
        if (m_data)
        {
            if (m_cleaner)
                m_cleaner(m_data);
            else
                std::cout << "WARNING: untype container has been destroyed with allocated data! Lost " << m_capacity << " x " << m_entrySize << " bytes, " << m_capacity * m_entrySize << " bytes total\n";
        }
    }

private:
    template<typename T>
    void realloc(size_t newCapacity)
    {
        T* newData = new T[newCapacity];
        T* oldData = static_cast<T*>(m_data);
        if (m_data)
        {
            auto maxid = std::min(newCapacity - 1, m_size - 1);
            for (int i = 0; i <= maxid; ++i)
                newData[i] = std::move(oldData[i]);

            delete[] oldData;
            m_data = newData;
            m_size = maxid + 1;
            m_capacity = newCapacity;
        }
    }

    void *m_data = nullptr;
    size_t m_capacity = 0;
    size_t m_size = 0;
    size_t m_entrySize = 0;

    void (*m_cleaner)(void*) = nullptr;
};

template<typename TReg>
class Archetype
{
public:
    // Should be called only once after creation
    template<typename... Ts>
    void addTypes(int reserve_)
    {
        (addType<Ts>(reserve_), ...);
    }

    template<typename T>
    bool containsEntity() const
    {
        return m_map.contains(TReg::template Get<T>());
    }

    inline size_t size() const
    {
        return m_map.begin()->second.size();
    }

    template<typename... Ts>
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

    void dumpAll()
    {
        auto sz = size();
        for (int i = 0; i < sz; ++i)
            dumpEntity(i);
    }

    void dumpEntity(size_t ent_)
    {
        dumpEntityComponents<1, TReg::MaxID>(ent_);

        std::cout << std::endl;
    }

    std::unordered_map<int, UntypeContainer> m_map;

private:
    template<typename T>
    void addType(int reserve_)
    {
        m_map[TReg::template Get<T>()].template allocate<T>(reserve_);
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

int main(int argc, char* args[])
{
    std::cout << "Hello\n";

    utils::dumpType(std::cout, typeid(Components).name());

    /*UntypeContainer comps(5, static_cast<ComponentA*>(nullptr));

    comps.push_back<ComponentA>(ComponentA(1));
    comps.push_back<ComponentA>(ComponentA(2));
    comps.push_back<ComponentA>(ComponentA(3));
    comps.push_back<ComponentA>(ComponentA(4));
    comps.push_back<ComponentA>(ComponentA(5));
    comps.reposLastTo<ComponentA>(2);
    comps.push_back<ComponentA>(ComponentA(6));
    comps.push_back<ComponentA>(ComponentA(7));
    comps.push_back<ComponentA>(ComponentA(8));
    comps.push_back<ComponentA>(ComponentA(9));
    comps.push_back<ComponentA>(ComponentA(10));
    comps.reposLastTo<ComponentA>(7); // 1 2 5 4 6 7 8 10

    for (int i = 0; i < comps.size(); ++i)
    {
        std::cout << comps.get<ComponentA>(i).value << " ";
    }*/

    Archetype<Components> arch;
    arch.addTypes<ComponentTransform, ComponentPhysical>(5);
    arch.addEntity(ComponentTransform(Vector2<float>(2.0f, 3.3f), Vector2<float>(10.0f, 20.0f)), ComponentPhysical(Collider(Vector2{10.0f, 10.0f}, Vector2{100.0f, 50.0f}), 9.8f));

    arch.dumpEntity(0);
    std::cout << "\nEnding\n";
    
}
