#include "Utils.h"
#include "TypeManip.hpp"
#include "ExampleComponents.h"
#include "yaECS.hpp"
#include <iostream>
#include <unordered_map>
#include <exception>

using Components = TypeManip::TypeRegistry<>::Create
    ::Add<ComponentTransform>
    ::Add<ComponentPhysical>
    ::Add<ComponentCharacter>
    ::Add<ComponentPlayerInput>
    ::Add<ComponentMobNavigation>;

int main(int argc, char* args[])
{
    std::cout << "Hello\n";

    utils::dumpType(std::cout, typeid(Components).name());
    std::cout << std::endl;

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

    ECS::Archetype<Components> arch;
    arch.addTypes<ComponentTransform, ComponentPhysical>(5);

    for (int i = 0; i < 10; ++i)
    {
        auto ent = arch.addEntity();
        arch.emplaceComponents(ent, ComponentPhysical(Collider(i, i, 10, 10), 9.8), ComponentTransform(Vector2{float(i) + (rand() % 100) / 100.0f, 5.0f}, Vector2{1.0f, 1.0f}));
    }
    arch.dumpAll();

    auto view = arch.makeView<ComponentTransform, ComponentPhysical>(2);
    auto [trans_, phys_] = arch.makeView<ComponentTransform, ComponentPhysical>(2);
    std::cout << typeid(view).name() << std::endl;
    std::cout << typeid(trans_).name() << std::endl;
    std::cout << typeid(phys_).name() << std::endl;
    phys_.m_gravity = 100.0f;
    trans_.m_size = {999.999f, 999.999f};
    arch.dumpAll();

    std::cout << " === \n";
    arch.removeEntity(5);
    arch.removeEntity(2);
    arch.removeEntity(7);
    arch.dumpAll();

    std::cout << " === \n";
    arch.dumpAll();

    std::cout << "\nEnding\n";

    ECS::Registry<Components> reg;
    reg.createEntity<ComponentTransform, ComponentMobNavigation, ComponentCharacter>();
    reg.createEntity<ComponentTransform>();
    reg.createEntity<ComponentTransform, ComponentMobNavigation, ComponentCharacter>();
    reg.createEntity<ComponentTransform, ComponentPhysical, ComponentMobNavigation>();
    auto lastent = reg.createEntity<ComponentTransform>();
    reg.emplaceComponents(lastent, ComponentTransform({1.1f, 5.2f}, {10.0f, 1.0f}));

    ECS::Archetype<Components> arch0;
    arch0.addTypes<ComponentTransform, ComponentMobNavigation>(5);

    reg.dumpAll();
    lastent = reg.emplaceComponents<ComponentPhysical, ComponentPlayerInput>(lastent,  ComponentPhysical(Collider(5, 2, 10, 10), 9.8), ComponentPlayerInput());

    std::cout << "after edit:\n";
    reg.dumpAll();

    auto qr = reg.makeQuery<ComponentTransform, ComponentMobNavigation>();

    reg.removeComponents<ComponentPlayerInput>(lastent);
    reg.dumpAll();

}
