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
    reg.dump(0);

    PROCESS_MEMORY_COUNTERS memCounter;
    BOOL result = K32GetProcessMemoryInfo(GetCurrentProcess(), &memCounter, sizeof(memCounter));
    std::cout << "WorkingSetSize " << memCounter.WorkingSetSize / 1024 << std::endl;

    std::cout << reg.count<ComponentTransform, ComponentPhysical>() << std::endl;

    auto arr = reg.getTuples<ComponentTransform, ComponentPhysical>();

    std::cout << utils::normalizeType(typeid(arr).name()) << std::endl;
}
