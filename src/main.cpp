#include "ExampleComponents.h"
#include "yaECS.hpp"
#include <iostream>
#include <string>
#include "Utils.h"
#include "CoreComponents.h"
#include <array>
#include <vector>
#include "StateMachine.hpp"

using PlayerRef = ECS::EntityRef<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput>;

// Make an archetype list for example
using MyReg = ECS::ArchList<>
    ::add<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput, StateMachine<PlayerRef>>;

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

struct InputSystem
{
    InputSystem(ECS::Registry<MyReg> &reg_) :
        m_query{reg_.getQuery<ComponentPlayerInput>()}
    {
        
    }

    bool update()
    {
        bool l = false;
        bool r = false;
        std::string buf;
        std::getline(std::cin, buf);
        if (buf.contains('l') || buf.contains('L'))
            l = true;
        if (buf.contains('r') || buf.contains('R'))
            r = true;

        std::apply([&](auto&&... args) {
            ((
                updateCon(args.template get<ComponentPlayerInput>(), l, r)
                ), ...);
            }, m_query.m_tpl);

        return !buf.contains('q');
    }

    void updateCon(std::vector<ComponentPlayerInput> &input_, bool l_, bool r_)
    {
        for (int i = 0; i < input_.size(); ++i)
        {
            input_[i].m_inL = l_;
            input_[i].m_inR = r_;
        }
    }

    using PhysObjectsQuery = std::invoke_result_t<decltype(&ECS::Registry<MyReg>::getQuery<ComponentPlayerInput>), ECS::Registry<MyReg>>;

    PhysObjectsQuery m_query;
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
        // Its possible to check whether or not an archetype has a component at compile time
        /*
        if constexpr (T::template containsOne<ComponentCharacter>())
        {
            updateCon(arch.template get<ComponentTransform>(), arch.template get<ComponentCharacter>());
        }
        else
        {
            updateCon(arch.template get<ComponentTransform>());
        }
        */
        std::cout << typeid(T).name() << std::endl;
        for (int i = 0 ; i < arch.size(); ++i)
        {
            std::cout << arch.getEntity(i) << std::endl;
        }
    }

    using TransformObjectQuery = std::invoke_result_t<decltype(&ECS::Registry<MyReg>::getQuery<ComponentTransform>), ECS::Registry<MyReg>>;

    TransformObjectQuery m_tuples;
};

class StateRun : public GenericState<PlayerRef>
{
public:
    StateRun() :
        GenericState<PlayerRef>(TypeManip::TypeHash::getHash<decltype(*this)>(), "Run")
    {

    }

    inline virtual bool update(PlayerRef &owner_, uint32_t currentFrame_) override
    {
        GenericState<PlayerRef>::update(owner_, currentFrame_);

        auto &trans = std::get<ComponentTransform&>(owner_);
        auto &phys = std::get<ComponentPhysical&>(owner_);
        auto &inp = std::get<ComponentPlayerInput&>(owner_);

        if (inp.m_inL)
        {
            phys.m_velocity.x = -5.0f;
            trans.m_orientation = ORIENTATION::LEFT;
            return false;
        }
        else if (inp.m_inR)
        {
            phys.m_velocity.x = 5.0f;
            trans.m_orientation = ORIENTATION::RIGHT;
            return false;
        }
        else
        {
            std::cout << "Can leave\n";
            return true;
        }
    }

    inline virtual ORIENTATION isPossible(PlayerRef &owner_) const override
    {
        auto &inp = std::get<ComponentPlayerInput&>(owner_);
        if (inp.m_inL)
            return ORIENTATION::LEFT;
        else if (inp.m_inR)
            return ORIENTATION::RIGHT;

        return ORIENTATION::UNSPECIFIED;
    }
};

class StateIdle : public GenericState<PlayerRef>
{
public:
    StateIdle() :
        GenericState<PlayerRef>(TypeManip::TypeHash::getHash<decltype(*this)>(), "Idle")
    {
    }

};


struct PlayerStateSystem
{
public:
    PlayerStateSystem(ECS::Registry<MyReg> &reg_) :
        m_query{reg_.getQuery<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput, StateMachine<PlayerRef>>()}
    {
    }

    void initAll()
    {
        std::apply([&](auto&&... args) {
            ((
                (initArch(args))
                ), ...);
            }, m_query.m_tpl);
    }

    void updateAll()
    {
        std::apply([&](auto&&... args) {
            ((
                (updateArch(args))
                ), ...);
            }, m_query.m_tpl);
    }

    template<typename T>
    void initArch(T &arch_)
    {
        for (int i = 0; i < arch_.size(); ++i)
            initInstance(arch_.getEntity(i));
    }

    template<typename T2>
    void initInstance(T2 inst_)
    {
        auto &smc = std::get<StateMachine<PlayerRef>&>(inst_);
        smc.addState(std::unique_ptr<StateRun>(new StateRun()));
        smc.addState(std::unique_ptr<StateIdle>(new StateIdle()));
        smc.setInitialState(TypeManip::TypeHash::getHash<StateIdle>());
    }

    template<typename T>
    void updateArch(T &arch_)
    {
        auto &smcs = arch_.template get<StateMachine<PlayerRef>>();
        for (int i = 0; i < arch_.size(); ++i)
        {
            auto inst = arch_.template getEntity<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput>(i);
            auto &smc = smcs[i];
            smc.update(inst, 0);
        }
    }

private:
    using PlayersQuery = std::invoke_result_t<decltype(&ECS::Registry<MyReg>::getQuery<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput, StateMachine<PlayerRef>>), ECS::Registry<MyReg>>;
    PlayersQuery m_query;
};

int main(int argc, char* args[])
{
    ECS::Registry<MyReg> reg;
    reg.addEntity(ComponentTransform(), ComponentPhysical({2.3f, 39.9f, 10.0f, 15.0f}, 9.8f), ComponentCharacter("Nameless1", 1), ComponentPlayerInput(), StateMachine<PlayerRef>());

    PlayerStateSystem m_st(reg);
    PhysicsSystem phys(reg);
    RenderSystem ren(reg);
    InputSystem inp(reg);

    m_st.initAll();

    bool isRunning = true;

    while (isRunning)
    {
        isRunning = inp.update();
        m_st.updateAll();
        phys.update();
        ren.update();
    }

    /*ECS::Registry<MyReg> reg;
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


    reg.dump(0);*/
}
