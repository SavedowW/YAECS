#include "UntypeContainer.h"
#include <iostream>

ECS::UntypeContainer::UntypeContainer(UntypeContainer &&rhs_) :
    m_data(rhs_.m_data),
    m_capacity(rhs_.m_capacity),
    m_size(rhs_.m_size),
    m_entrySize(rhs_.m_entrySize),
    m_cleaner(rhs_.m_cleaner),
    m_callRealloc(rhs_.m_callRealloc),
    m_callRemoveAt(rhs_.m_callRemoveAt)
{
    rhs_.m_data = nullptr;
    rhs_.m_capacity = 0;
    rhs_.m_size = 0;
    rhs_.m_entrySize = 0;
    rhs_.m_cleaner = nullptr;
    rhs_.m_callRealloc = nullptr;
    rhs_.m_callRemoveAt = nullptr;
}

ECS::UntypeContainer &ECS::UntypeContainer::operator=(UntypeContainer &&rhs_)
{
    m_data = rhs_.m_data;
    m_capacity = rhs_.m_capacity;
    m_size = rhs_.m_size;
    m_entrySize = rhs_.m_entrySize;
    m_cleaner = rhs_.m_cleaner;
    m_callRealloc = rhs_.m_callRealloc;

    rhs_.m_data = nullptr;
    rhs_.m_capacity = 0;
    rhs_.m_size = 0;
    rhs_.m_entrySize = 0;
    rhs_.m_cleaner = nullptr;
    rhs_.m_callRealloc = nullptr;
    rhs_.m_callRemoveAt = nullptr;

    return *this;
}

size_t ECS::UntypeContainer::size() const
{
    return m_size;
}

void ECS::UntypeContainer::push_back()
{
    if (m_size == m_capacity)
        m_callRealloc(this, m_capacity * 1.3);

    ++m_size;
}

void ECS::UntypeContainer::removeAt(size_t newIdx_)
{
    m_callRemoveAt(this, newIdx_);
}

ECS::UntypeContainer::~UntypeContainer()
{
    if (m_data)
    {
        if (m_cleaner)
            m_cleaner(m_data);
        else
            std::cout << "WARNING: untype container has been destroyed with allocated data! Lost " << m_capacity << " x " << m_entrySize << " bytes, " << m_capacity * m_entrySize << " bytes total\n";
    }
}