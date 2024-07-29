#include "ExampleComponents.h"
#include "yaECS_Static.hpp"
#include <iostream>
#include <string>
#include "Utils.h"
#include "CoreComponents.h"
#include <array>
#include <vector>
#include "StateMachine.hpp"

enum class PlayerStates {
    IDLE,
    RUN,
    NONE
};

enum class MobStates {
    META_ROAM,
    META_CHASE,
    IDLE,
    WALK,
    RUN,
    NONE
};

using ArchPlayer = ECS_Static::EntityData<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput>;
using ArchMob = ECS_Static::EntityData<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentMobNavigation>;
using WorldObj = ECS_Static::EntityData<ComponentTransform, ComponentPhysical>;

// Make an archetype list for example
using MyReg = ECS_Static::ArchList<>
    ::addTypelist<ArchPlayer::WithSM<PlayerStates>>
    ::addTypelist<ArchMob::WithSM<MobStates>>
    ::add<WorldObj>;

/*
    Examples of systems
    Yes, you need to duplicate getQuery to specify field type and actually call it, I don't know what to do about it
    A system can get as much queries as it wants, there are no limitations
*/

struct PhysicsSystem
{
    PhysicsSystem(ECS_Static::Registry<MyReg> &reg_) :
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

    using PhysObjectsQuery = std::invoke_result_t<decltype(&ECS_Static::Registry<MyReg>::getQuery<ComponentTransform, ComponentPhysical>), ECS_Static::Registry<MyReg>>;

    PhysObjectsQuery m_tuples;
};

struct InputSystem
{
    InputSystem(ECS_Static::Registry<MyReg> &reg_) :
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

    using PhysObjectsQuery = std::invoke_result_t<decltype(&ECS_Static::Registry<MyReg>::getQuery<ComponentPlayerInput>), ECS_Static::Registry<MyReg>>;

    PhysObjectsQuery m_query;
};

struct RenderSystem
{
    RenderSystem(ECS_Static::Registry<MyReg> &reg_) :
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
        for (int i = 0 ; i < arch.size(); ++i)
        {
            std::cout << arch.getEntity(i) << std::endl;
        }
    }

    using TransformObjectQuery = std::invoke_result_t<decltype(&ECS_Static::Registry<MyReg>::getQuery<ComponentTransform>), ECS_Static::Registry<MyReg>>;

    TransformObjectQuery m_tuples;
};

class StateRun : public GenericState<ArchPlayer::MakeRef, PlayerStates>
{
public:
    StateRun(float horSpeed_) :
        GenericState<ArchPlayer::MakeRef, PlayerStates>(PlayerStates::RUN, "Run", {PlayerStates::NONE, {PlayerStates::IDLE, PlayerStates::RUN}}),
        m_horSpeed(horSpeed_)
    {

    }

    inline virtual void enter(ArchPlayer::MakeRef &owner_, PlayerStates from_) override
    {
        GenericState<ArchPlayer::MakeRef, PlayerStates>::enter(owner_, from_);

        auto &trans = std::get<ComponentTransform&>(owner_);
        auto &phys = std::get<ComponentPhysical&>(owner_);

        if (trans.m_orientation == ORIENTATION::LEFT)
        {
            phys.m_velocity.x = -m_horSpeed;
        }
        else if (trans.m_orientation == ORIENTATION::RIGHT)
        {
            phys.m_velocity.x = m_horSpeed;
        }
    }

    inline virtual bool update(ArchPlayer::MakeRef &owner_, uint32_t currentFrame_) override
    {
        GenericState<ArchPlayer::MakeRef, PlayerStates>::update(owner_, currentFrame_);

        auto &trans = std::get<ComponentTransform&>(owner_);
        auto &inp = std::get<ComponentPlayerInput&>(owner_);

        if (inp.m_inL && trans.m_orientation == ORIENTATION::LEFT || inp.m_inR && trans.m_orientation == ORIENTATION::RIGHT)
            return false;
        else
        {
            std::cout << "Can leave\n";
            return true;
        }
    }

    inline virtual ORIENTATION isPossible(ArchPlayer::MakeRef &owner_) const override
    {
        auto &inp = std::get<ComponentPlayerInput&>(owner_);
        if (inp.m_inL)
            return ORIENTATION::LEFT;
        else if (inp.m_inR)
            return ORIENTATION::RIGHT;

        return ORIENTATION::UNSPECIFIED;
    }

protected:
    int m_horSpeed;
};

template<typename Owner_t, typename PLAYER_STATE_T>
class StateIdle : public GenericState<Owner_t, PLAYER_STATE_T>
{
public:
    StateIdle(PLAYER_STATE_T state_, StateMarker<PLAYER_STATE_T> &&transitionableFrom_) :
        GenericState<Owner_t, PLAYER_STATE_T>(state_, "Idle", std::move(transitionableFrom_))
    {
    }

    inline virtual void enter(Owner_t &owner_, PLAYER_STATE_T from_)
    {
        auto &phys = std::get<ComponentPhysical&>(owner_);
        phys.m_velocity.x = 0;
    }

};


class StateMobNavigation : public GenericState<ArchMob::MakeRef, MobStates>
{
public:
    StateMobNavigation(MobStates state_, float horSpeed_, StateMarker<MobStates> &&transitionableFrom_) :
        GenericState<ArchMob::MakeRef, MobStates>(state_, "Move dir", std::move(transitionableFrom_)),
        m_horSpeed(horSpeed_)
    {

    }

    inline virtual void enter(ArchMob::MakeRef &owner_, MobStates from_) override
    {
        GenericState<ArchMob::MakeRef, MobStates>::enter(owner_, from_);

        auto &trans = std::get<ComponentTransform&>(owner_);
        auto &phys = std::get<ComponentPhysical&>(owner_);
        auto &mobdata = std::get<ComponentMobNavigation&>(owner_);

        if (mobdata.dir > 0)
        {
            trans.m_orientation = ORIENTATION::LEFT;
            phys.m_velocity.x = -m_horSpeed;
        }
        else
        {
            trans.m_orientation = ORIENTATION::RIGHT;
            phys.m_velocity.x = m_horSpeed;
        }
    }

    inline virtual bool update(ArchMob::MakeRef &owner_, uint32_t currentFrame_) override
    {
        GenericState<ArchMob::MakeRef, MobStates>::update(owner_, currentFrame_);

        auto &mobdata = std::get<ComponentMobNavigation&>(owner_);
        return (mobdata.framesLeft-- <= 0);
    }

    inline virtual ORIENTATION isPossible(ArchMob::MakeRef &owner_) const override
    {
        auto &trans = std::get<ComponentTransform&>(owner_);
        auto &mobdata = std::get<ComponentMobNavigation&>(owner_);
        if (mobdata.framesLeft > 0 && mobdata.dir != 0)
            return (mobdata.dir > 0 ? ORIENTATION::RIGHT : ORIENTATION::LEFT);
        else
            return ORIENTATION::UNSPECIFIED;
    }

protected:
    int m_horSpeed;
};

class StateMobMetaRoam : public NodeState<ArchMob::MakeRef, MobStates>
{
public:
    StateMobMetaRoam() :
        NodeState<ArchMob::MakeRef, MobStates>(MobStates::META_ROAM, "ROAM", {MobStates::NONE, {MobStates::META_CHASE}})
    {
    }

    inline virtual void enter(ArchMob::MakeRef &owner_, MobStates from_) override
    {
        auto &mobdata = std::get<ComponentMobNavigation&>(owner_);
        mobdata.framesLeft = 10;
        mobdata.dir = (rand() % 2) * 2 - 1;
        switchCurrentState(owner_, MobStates::WALK);
    }

    virtual bool update(ArchMob::MakeRef &owner_, uint32_t currentFrame_) override
    {
        auto &mobdata = std::get<ComponentMobNavigation&>(owner_);

        auto res = NodeState<ArchMob::MakeRef, MobStates>::update(owner_, currentFrame_);
        if (res)
        {
            if (m_currentState->m_stateId == MobStates::IDLE)
            {
                mobdata.framesLeft = 5;
                mobdata.dir = 0;
            }
        }
        else if (m_currentState->m_stateId == MobStates::IDLE)
        {
            if (mobdata.framesLeft == 0)
            {
                mobdata.framesLeft = 10;
                mobdata.dir = (rand() % 2) * 2 - 1;
                switchCurrentState(owner_, MobStates::WALK);
            }
            else
            {
                mobdata.framesLeft--;
                std::cout << "Reduced to " << mobdata.framesLeft << std::endl;
            }
        }

        if (currentFrame_ >= 10)
            return true;
        
        return false;
        
    }

};

class StateMobMetaChase : public NodeState<ArchMob::MakeRef, MobStates>
{
public:
    StateMobMetaChase() :
        NodeState<ArchMob::MakeRef, MobStates>(MobStates::META_CHASE, "CHASE", {MobStates::NONE, {MobStates::META_ROAM}})
    {
    }

    inline virtual void enter(ArchMob::MakeRef &owner_, MobStates from_) override
    {
        switchCurrentState(owner_, MobStates::RUN);
    }

    virtual bool update(ArchMob::MakeRef &owner_, uint32_t currentFrame_) override
    {
        auto res = NodeState<ArchMob::MakeRef, MobStates>::update(owner_, currentFrame_);

        if (currentFrame_ >= 15)
            return true;
        
        return false;
    }

};

struct PlayerStateSystem
{
public:
    PlayerStateSystem(ECS_Static::Registry<MyReg> &reg_) :
        m_query{reg_.getQueryTl(ArchPlayer::WithSM<PlayerStates>())}
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
        auto &smc = std::get<StateMachine<ArchPlayer::MakeRef, PlayerStates>&>(inst_);
        smc.addState(std::unique_ptr<StateRun>(new StateRun(5.0f)));
        smc.addState(std::unique_ptr<StateIdle<ArchPlayer::MakeRef, PlayerStates>>(new StateIdle<ArchPlayer::MakeRef, PlayerStates>(PlayerStates::IDLE, {PlayerStates::NONE, {PlayerStates::RUN}})));
        smc.setInitialState(PlayerStates::IDLE);
    }

    template<typename T>
    void updateArch(T &arch_)
    {
        auto &smcs = arch_.template get<StateMachine<ArchPlayer::MakeRef, PlayerStates>>();
        for (int i = 0; i < arch_.size(); ++i)
        {
            auto inst = arch_.template getEntity<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput>(i);
            auto &smc = smcs[i];
            smc.update(inst, 0);
        }
    }

private:
    using PlayersQuery = std::invoke_result_t<decltype(&ECS_Static::Registry<MyReg>::getQueryTl<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput, StateMachine<ArchPlayer::MakeRef, PlayerStates>>), ECS_Static::Registry<MyReg>, ArchPlayer::WithSM<PlayerStates>>;
    PlayersQuery m_query;
};


struct MobStateSystem
{
public:
    MobStateSystem(ECS_Static::Registry<MyReg> &reg_) :
        m_query{reg_.getQueryTl(ArchMob::WithSM<MobStates>())}
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
        auto &smc = std::get<StateMachine<ArchMob::MakeRef, MobStates>&>(inst_);

        auto tmproam = std::unique_ptr<StateMobMetaRoam>(new StateMobMetaRoam());
        tmproam->addState(std::unique_ptr<StateMobNavigation>(new StateMobNavigation(MobStates::WALK, 2.0f, {MobStates::NONE, {MobStates::IDLE}})));
        tmproam->addState(std::unique_ptr<StateIdle<ArchMob::MakeRef, MobStates>>(new StateIdle<ArchMob::MakeRef, MobStates>(MobStates::IDLE, {MobStates::NONE, {MobStates::WALK, MobStates::RUN}})));
        tmproam->setInitialState(MobStates::IDLE);
    
        auto tmpchase = std::unique_ptr<StateMobMetaChase>(new StateMobMetaChase());
        tmpchase->addState(std::unique_ptr<StateMobNavigation>(new StateMobNavigation(MobStates::RUN, 5.0f, {MobStates::NONE, {MobStates::IDLE, MobStates::WALK}})));
        tmpchase->setInitialState(MobStates::RUN);
    
        smc.addState(std::move(tmproam));
        smc.addState(std::move(tmpchase));
    
        smc.setInitialState(MobStates::META_ROAM);
    }

    template<typename T>
    void updateArch(T &arch_)
    {
        auto &smcs = arch_.template get<StateMachine<ArchMob::MakeRef, MobStates>>();
        for (int i = 0; i < arch_.size(); ++i)
        {
            auto inst = arch_.template getEntity<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentMobNavigation>(i);
            auto &smc = smcs[i];
            smc.update(inst, 0);
        }
    }

private:
    using MobsQuery = std::invoke_result_t<decltype(
        &ECS_Static::Registry<MyReg>::getQueryTl<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentMobNavigation, StateMachine<ArchMob::MakeRef, MobStates>>
        ), ECS_Static::Registry<MyReg>, ArchMob::WithSM<MobStates>>;
    MobsQuery m_query;
};


int main(int argc, char* args[])
{
    //ECS_Static::Archetype<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput> myarch1;
    //ECS_Static::Archetype<ComponentTransform, ComponentPhysical, ComponentMobNavigation> myarch2;
    //ECS_Static::ViewQuery<2, ComponentTransform, ComponentPhysical> qr({myarch1.getView<ComponentTransform, ComponentPhysical>(), myarch2.getView<ComponentTransform, ComponentPhysical>()});

    ECS_Static::Registry<MyReg> reg;
    reg.addEntity(ComponentTransform(), ComponentPhysical({2.3f, 39.9f, 10.0f, 15.0f}, 9.8f), ComponentCharacter("Nameless1", 1), ComponentPlayerInput(), StateMachine<ArchPlayer::MakeRef, PlayerStates>());
    reg.addEntity(ComponentTransform(), ComponentPhysical({2.3f, 39.9f, 10.0f, 15.0f}, 0.1f), ComponentCharacter("Scary Skeleton", 3), ComponentMobNavigation(), StateMachine<ArchMob::MakeRef, MobStates>());

    auto qr = reg.getQuery<ComponentTransform, ComponentCharacter>();
    auto view = qr.makeView<ComponentTransform, ComponentCharacter>();
    utils::dumpType(std::cout, typeid(view).name());

    PlayerStateSystem m_st(reg);
    MobStateSystem m_mbst(reg);
    PhysicsSystem phys(reg);
    RenderSystem ren(reg);
    InputSystem inp(reg);

    m_st.initAll();
    m_mbst.initAll();

    bool isRunning = true;

    while (isRunning)
    {
        isRunning = inp.update();
        m_st.updateAll();
        m_mbst.updateAll();
        phys.update();
        ren.update();
    }

    /*ECS_Static::Registry<MyReg> reg;
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
            reg.convert<ECS_Static::Archetype<ComponentTransform, ComponentPhysical, ComponentCharacter>, ECS_Static::Archetype<ComponentTransform, ComponentCharacter>>(0);
        phys.update();
        ren.update();
        std::cin >> cmd;
    }


    reg.dump(0);*/
}
