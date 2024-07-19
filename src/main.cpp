#include "ExampleComponents.h"
#include "ECS.hpp"
#include <iostream>
#include <string>
#include "Utils.h"
#include "Collider.h"
#include "CoreComponents.h"
#include <array>
#include <vector>
#include <windows.h>
#include <Psapi.h>

using MyReg = ArchList<>
    ::add<ComponentTransform, ComponentPhysical, ComponentCharacter>
    ::add<ComponentTransform, ComponentPhysical>
    ::add<ComponentTransform, ComponentCharacter>;

struct PhysicsSystem
{
    PhysicsSystem(Registry<MyReg> &reg_) :
        m_tuples{reg_.getTuples<ComponentTransform, ComponentPhysical>()}
    {
        
    }

    void update()
    {
        std::apply([&](auto&&... args) {
            ((
                updateCon(args.get<ComponentTransform>(), args.get<ComponentPhysical>())
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

    using PhysObjectsQuery = return_type_t< decltype(&Registry<MyReg>::getTuples<ComponentTransform, ComponentPhysical>) >;

    PhysObjectsQuery m_tuples;
};

struct RenderSystem
{
    RenderSystem(Registry<MyReg> &reg_) :
        m_tuples{reg_.getTuples<ComponentTransform>()}
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
        if constexpr (arch.containsOne<ComponentCharacter>())
        {
            updateCon(arch.get<ComponentTransform>(), arch.get<ComponentCharacter>());
        }
        else
        {
            updateCon(arch.get<ComponentTransform>());
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

    using TransformObjectQuery = return_type_t< decltype(&Registry<MyReg>::getTuples<ComponentTransform>) >;

    TransformObjectQuery m_tuples;
};

int main(int argc, char* args[])
{
    Registry<MyReg> reg;
    std::cout << reg.addEntity(ComponentTransform{{2.3f, -99.5f}, {1.0f, 2.0f}}, ComponentPhysical{{2.3f, 39.9f, 10.0f, 15.0f}, 9.8f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{1.1f, 1.2f}, {1.1f, 1.2f}}, ComponentPhysical{{1.1f, 1.2f, 1.3f, 1.4f}, 9.9f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{2.1f, 2.2f}, {2.1f, 2.2f}}, ComponentPhysical{{2.1f, 2.2f, 2.3f, 2.4f}, 10.0f}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{3.1f, 3.2f}, {3.1f, 3.2f}}, ComponentPhysical{{3.1f, 3.2f, 3.3f, 3.4f}, 11.0f}) << std::endl;

    std::cout << reg.addEntity(ComponentTransform{{4.1f, 4.2f}, {4.1f, 4.2f}}, ComponentPhysical{{4.1f, 4.2f, 4.3f, 4.4f}, 12.0f}) << std::endl;

    std::cout << reg.addEntity(ComponentTransform{{-100.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{3.1f, 3.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless2", 7}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-100.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{4.1f, 4.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless3", 1}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-100.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{4.1f, 4.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless4", 1}) << std::endl;
    std::cout << reg.addEntity(ComponentTransform{{-100.99f, -100.99f}, {1.0f, 2.0f}}, ComponentPhysical{{4.1f, 4.2f, 3.3f, 3.4f}, 11.0f}, ComponentCharacter{"Nameless5", 1}) << std::endl;
    reg.dump(0);

    PhysicsSystem phys(reg);
    RenderSystem ren(reg);

    int cmd = 1;
    while (cmd)
    {
        phys.update();
        ren.update();
        std::cin >> cmd;
    }

    
}
