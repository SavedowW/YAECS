#include "ExampleComponents.h"
#include "yaECS.hpp"
#include <iostream>
#include <string>
#include "Utils.h"
#include "Collider.h"
#include "CoreComponents.h"
#include <array>
#include <vector>
#include <windows.h>
#include <Psapi.h>

// Make an archetype list for example
using MyReg = ECS::ArchList<>
    ::add<ComponentTransform, ComponentPhysical, ComponentCharacter>
    ::add<ComponentTransform, ComponentPhysical>
    ::add<ComponentTransform, ComponentCharacter>;

/*
    Examples of systems
    Yes, you need to duplicate getQuery to specify field type and actually call it, I don't know what to do about it
    A system can get as much queries as it wants, there are no limitations
*/

struct PhysicsSystem
{
    PhysicsSystem(ECS::Registry<MyReg> &reg_) :
        m_tuples{reg_.getQuery<ComponentTransform, ComponentPhysical>()}
    {
        
    }

    void update()
    {
        std::apply([&](auto&&... args) {
            ((
                updateCon(args.template get<ComponentTransform>(), args.template get<ComponentPhysical>())
                ), ...);
            }, m_tuples.m_tpl);
    }

    void updateCon(std::vector<ComponentTransform> &transform_, std::vector<ComponentPhysical> &physical_)
    {
        for (int i = 0; i < transform_.size(); ++i)
        {
            physical_[i].m_velocity.y += physical_[i].m_gravity;
            transform_[i].m_pos += physical_[i].m_velocity;
        }
    }

    using PhysObjectsQuery = std::invoke_result_t<decltype(&ECS::Registry<MyReg>::getQuery<ComponentTransform, ComponentPhysical>), ECS::Registry<MyReg>>;

    PhysObjectsQuery m_tuples;
};

struct RenderSystem
{
    RenderSystem(ECS::Registry<MyReg> &reg_) :
        m_tuples{reg_.getQuery<ComponentTransform>()}
    {
        
    }

    void update()
    {
        std::apply([&](auto&&... args) {
            ((
                (updateDistrib(args))
                ), ...);
            }, m_tuples.m_tpl);
    }

    template<typename T>
    void updateDistrib(T &arch)
    {
        if constexpr (T::template containsOne<ComponentCharacter>())
        {
            updateCon(arch.template get<ComponentTransform>(), arch.template get<ComponentCharacter>());
        }
        else
        {
            updateCon(arch.template get<ComponentTransform>());
        }
    }

    void updateCon(std::vector<ComponentTransform> &transform_)
    {
        std::cout << "Transform only\n";
        for (int i = 0; i < transform_.size(); ++i)
        {
            std::cout << transform_[i] << std::endl;
        }
    }

    void updateCon(std::vector<ComponentTransform> &transform_, std::vector<ComponentCharacter> &character_)
    {
        std::cout << "Transform and character\n";
        for (int i = 0; i < transform_.size(); ++i)
        {
            std::cout << transform_[i] << " : " << character_[i] << std::endl;
        }
    }

    using TransformObjectQuery = std::invoke_result_t<decltype(&ECS::Registry<MyReg>::getQuery<ComponentTransform>), ECS::Registry<MyReg>>;

    TransformObjectQuery m_tuples;
};


int main(int argc, char* args[])
{
    ECS::Registry<MyReg> reg;
    std::cout << reg.addEntity(ComponentTransform{{2.3f, -99.5f}, {1.0f, 2.0f}}, ComponentPhysical{{2.3f, 39.9f, 10.0f, 15.0f}, 9.8f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{1.1f, 1.2f}, {1.1f, 1.2f}}, ComponentPhysical{{1.1f, 1.2f, 1.3f, 1.4f}, 9.9f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{2.1f, 2.2f}, {2.1f, 2.2f}}, ComponentPhysical{{2.1f, 2.2f, 2.3f, 2.4f}, 10.0f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{3.1f, 3.2f}, {3.1f, 3.2f}}, ComponentPhysical{{3.1f, 3.2f, 3.3f, 3.4f}, 11.0f}) << std::endl;

    std::cout << reg.addEntity(ComponentTransform{{4.1f, 4.2f}, {4.1f, 4.2f}}, ComponentPhysical{{4.1f, 4.2f, 4.3f, 4.4f}, 12.0f}) << std::endl;

    std::cout << reg.addEntity(ComponentTransform{{-101.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{3.1f, 3.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless1", 1}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-102.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{4.1f, 4.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless2", 2}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-103.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{4.1f, 4.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless3", 3}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-104.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{4.1f, 4.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless4", 4}) << std::endl;
    reg.dump(0);

    PhysicsSystem phys(reg);
    RenderSystem ren(reg);

    int cmd = 1;
    while (cmd)
    {
        if (cmd > 5)
            reg.convert<ECS::Archetype<ComponentTransform, ComponentPhysical, ComponentCharacter>, ECS::Archetype<ComponentTransform, ComponentCharacter>>(0);
        phys.update();
        ren.update();
        std::cin >> cmd;
    }


    reg.dump(0);
}
