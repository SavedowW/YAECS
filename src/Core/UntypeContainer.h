#ifndef UNTYPE_CONTAINER_H_
#define UNTYPE_CONTAINER_H_
#include <utility>
#include <algorithm>

namespace ECS
{
    /*
        Container for optional data type
        Stores data linearly as raw bytes, converts only on access
        When a field is deleted, moved last field to it instead of moving entire array
        TODO: might make it just an interface with an actual object knowing about type
        Virtual calls are a bit faster than calls to lambdas through interface plus it will allow some optimization
        because currently container does not know its type and registry / archetype often need to iterate over all components
    */
    class UntypeContainer
    {
    public:
        UntypeContainer() = default;

        UntypeContainer(const UntypeContainer &rhs_) = delete;
        UntypeContainer &operator=(const UntypeContainer &rhs_) = delete;
        UntypeContainer(UntypeContainer &&rhs_);
        UntypeContainer &operator=(UntypeContainer &&rhs_);

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

            m_callRealloc = [](UntypeContainer *container_, size_t newCapacity_)
            {
                container_->realloc<T>(newCapacity_);
            };

            m_callRemoveAt = [](UntypeContainer *container_, size_t id_)
            {
                container_->removeAt<T>(id_);
            };

            return true;
        }

        template<typename T>
        bool allocated() const
        {
            return m_data;
        }

        template<typename T>
        bool freemem()
        {
            if (!m_data)
                return false;

            delete []static_cast<T*>(m_data);
            m_entrySize = 0;
            m_capacity = 0;
            m_data = nullptr;
            m_cleaner = nullptr;
            m_callRealloc = nullptr;
            m_callRemoveAt = nullptr;

            return true;
        }

        template<typename T>
        T &get(std::size_t id_)
        {
            return static_cast<T*>(m_data)[id_];
        }

        std::size_t size() const;

        template <typename T>
        void push_back(T &&rhs_)
        {
            if (m_size == m_capacity)
                realloc<T>(m_capacity * 1.3);

            static_cast<T*>(m_data)[m_size] = std::move(rhs_);
            ++m_size;
        }

        void push_back();

        template <typename T>
        void emplace(T &&rhs_, std::size_t id_)
        {
            static_cast<T*>(m_data)[id_] = std::move(rhs_);
        }

        template<typename T>
        bool removeAt(std::size_t newIdx_)
        {
            if (newIdx_ > m_size - 1)
                return false;
            else if (newIdx_ == m_size - 1)
            {
                --m_size;
                return true;
            }
            else
            {
                auto *realarr = static_cast<T*>(m_data);
                --m_size;
                realarr[newIdx_] = std::move(realarr[m_size]);
                return true;
            }
        }

        void removeAt(std::size_t newIdx_);

        ~UntypeContainer();

    private:
        template<typename T>
        void realloc(std::size_t newCapacity_)
        {
            T* oldData = static_cast<T*>(m_data);
            if (m_data)
            {
                T* newData = new T[newCapacity_];
                auto maxid = std::min(newCapacity_ - 1, m_size - 1);
                for (int i = 0; i <= maxid; ++i)
                    newData[i] = std::move(oldData[i]);

                delete[] oldData;
                m_data = newData;
                m_size = maxid + 1;
                m_capacity = newCapacity_;
            }
        }

        void *m_data = nullptr;
        std::size_t m_capacity = 0; // Total amount of allocated elements
        std::size_t m_size = 0; // Amount of used elements
        std::size_t m_entrySize = 0; // Size of a single element

        // Lambdas that know internal type and use it
        void (*m_cleaner)(void*) = nullptr;
        void (*m_callRealloc)(UntypeContainer *container_, std::size_t newCapacity_) = nullptr;
        void (*m_callRemoveAt)(UntypeContainer *container_, std::size_t id_) = nullptr;
    };
}

#endif
