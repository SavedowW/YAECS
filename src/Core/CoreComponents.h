#ifndef CORE_COMPONENTS_H_
#define CORE_COMPONENTS_H_
#include "ComponentEntity.hpp"
#include "Vector2.h"

namespace Components
{
    struct Transform : public Component<>
    {
        Transform() = default;

        Vector2<float> m_pos;
    };
}

#endif