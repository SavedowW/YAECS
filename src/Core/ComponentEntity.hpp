#ifndef COMPONENT_ENTITY_H_
#define COMPONENT_ENTITY_H_
#include <tuple>
#include <algorithm>
#include <iostream>

namespace Components
{

    template<typename ...Args2>
    class Component
    {
    public:
        Component()
        {}

        void resolveDeps(Args2*... args_)
        {
            m_deps = {args_...};
        }

        template<typename T>
        T &getComponent()
        {
            return *std::get<T*>(m_deps);
        }

        std::tuple<Args2*...> m_deps;
    };

    template<typename ...Args>
    class Entity
    {
    public:
        Entity(Args&&... args_) :
            m_components(std::forward<Args>(args_)...)
        {
            int i = 0;
            std::cout << "Created " << typeid(*this).name() << std::endl;
        }

        template<typename T>
        T &getComponent()
        {
            return std::get<T>(m_components);
        }

        template<typename T>
        const T &getComponent() const
        {
            return std::get<T>(m_components);
        }

    protected:
        virtual void resolveDeps() = 0;
        std::tuple<Args...> m_components;
    };

}

#endif