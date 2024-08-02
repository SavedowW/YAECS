#include "Utils.h"
#include "MultipleCall.hpp"
#include "TypeManip.hpp"
#include "ExampleComponents.h"
#include "yaECS.hpp"
#include "StateMachine.h"
#include <iostream>
#include <unordered_map>
#include <exception>

using Components = TypeManip::TypeRegistry<>::Create
    ::Add<ComponentTransform>
    ::Add<ComponentPhysical>
    ::Add<ComponentCharacter>
    ::Add<ComponentPlayerInput>
    ::Add<ComponentMobNavigation>
    ::Add<StateMachine>;

enum class PlayerStates : CharState {
    IDLE,
    RUN,
    NONE
};

enum class MobStates : CharState {
    META_ROAM,
    META_CHASE,
    IDLE,
    WALK,
    RUN,
    NONE
};


// Examples of states and state machines

class StateRun : public GenericState
{
public:
    StateRun(float horSpeed_) :
        GenericState(PlayerStates::RUN, "Run", {PlayerStates::NONE, {PlayerStates::IDLE, PlayerStates::RUN}}),
        m_horSpeed(horSpeed_)
    {

    }

    inline virtual void enter(ECS::EntityView &owner_, CharState from_) override
    {
        GenericState::enter(owner_, from_);

        auto &trans = owner_.get<Components, ComponentTransform>();
        auto &phys = owner_.get<Components, ComponentPhysical>();

        if (trans.m_orientation == ORIENTATION::LEFT)
        {
            phys.m_velocity.x = -m_horSpeed;
        }
        else if (trans.m_orientation == ORIENTATION::RIGHT)
        {
            phys.m_velocity.x = m_horSpeed;
        }
    }

    inline virtual bool update(ECS::EntityView &owner_, uint32_t currentFrame_) override
    {
        GenericState::update(owner_, currentFrame_);

        auto &trans = owner_.get<Components, ComponentTransform>();
        auto &inp = owner_.get<Components, ComponentPlayerInput>();

        if (inp.m_inL && trans.m_orientation == ORIENTATION::LEFT || inp.m_inR && trans.m_orientation == ORIENTATION::RIGHT)
            return false;
        else
        {
            std::cout << "Can leave\n";
            return true;
        }
    }

    inline virtual ORIENTATION isPossible(ECS::EntityView &owner_) const override
    {
        auto &inp = owner_.get<Components, ComponentPlayerInput>();

        if (inp.m_inL)
            return ORIENTATION::LEFT;
        else if (inp.m_inR)
            return ORIENTATION::RIGHT;

        return ORIENTATION::UNSPECIFIED;
    }

protected:
    int m_horSpeed;
};

template<typename PLAYER_STATE_T>
class StateIdle : public GenericState
{
public:
    StateIdle(PLAYER_STATE_T state_, StateMarker &&transitionableFrom_) :
        GenericState(state_, "Idle", std::move(transitionableFrom_))
    {
    }

    inline virtual void enter(ECS::EntityView &owner_, CharState from_)
    {
        if (owner_.contains<Components, ComponentPhysical>())
        {
            auto &phys = owner_.get<Components, ComponentPhysical>();
            phys.m_velocity.x = 0;
        }
    }
};


class StateMobNavigation : public GenericState
{
public:
    StateMobNavigation(MobStates state_, float horSpeed_, StateMarker &&transitionableFrom_) :
        GenericState(state_, "Move dir", std::move(transitionableFrom_)),
        m_horSpeed(horSpeed_)
    {

    }

    inline virtual void enter(ECS::EntityView &owner_, CharState from_) override
    {
        GenericState::enter(owner_, from_);

        auto &trans = owner_.get<Components, ComponentTransform>();
        auto &phys = owner_.get<Components, ComponentPhysical>();
        auto &mobdata = owner_.get<Components, ComponentMobNavigation>();

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

    inline virtual bool update(ECS::EntityView &owner_, uint32_t currentFrame_) override
    {
        GenericState::update(owner_, currentFrame_);

        auto &mobdata = owner_.get<Components, ComponentMobNavigation>();
        return (mobdata.framesLeft-- <= 0);
    }

    inline virtual ORIENTATION isPossible(ECS::EntityView &owner_) const override
    {
        auto &trans = owner_.get<Components, ComponentTransform>();
        auto &mobdata = owner_.get<Components, ComponentMobNavigation>();
        if (mobdata.framesLeft > 0 && mobdata.dir != 0)
            return (mobdata.dir > 0 ? ORIENTATION::RIGHT : ORIENTATION::LEFT);
        else
            return ORIENTATION::UNSPECIFIED;
    }

protected:
    int m_horSpeed;
};

class StateMobMetaRoam : public NodeState
{
public:
    StateMobMetaRoam() :
        NodeState(MobStates::META_ROAM, "ROAM", {MobStates::NONE, {MobStates::META_CHASE}})
    {
    }

    inline virtual void enter(ECS::EntityView &owner_, CharState from_) override
    {
        auto &mobdata = owner_.get<Components, ComponentMobNavigation>();
        mobdata.framesLeft = 10;
        mobdata.dir = (rand() % 2) * 2 - 1;
        switchCurrentState(owner_, MobStates::WALK);
    }

    virtual bool update(ECS::EntityView &owner_, uint32_t currentFrame_) override
    {
        auto &mobdata = owner_.get<Components, ComponentMobNavigation>();

        auto res = NodeState::update(owner_, currentFrame_);
        if (res)
        {
            if (m_currentState->m_stateId == static_cast<CharState>(MobStates::IDLE))
            {
                mobdata.framesLeft = 5;
                mobdata.dir = 0;
            }
        }
        else if (m_currentState->m_stateId == static_cast<CharState>(MobStates::IDLE))
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

class StateMobMetaChase : public NodeState
{
public:
    StateMobMetaChase() :
        NodeState(MobStates::META_CHASE, "CHASE", {MobStates::NONE, {MobStates::META_ROAM}})
    {
    }

    inline virtual void enter(ECS::EntityView &owner_, CharState from_) override
    {
        switchCurrentState(owner_, MobStates::RUN);
    }

    virtual bool update(ECS::EntityView &owner_, uint32_t currentFrame_) override
    {
        auto res = NodeState::update(owner_, currentFrame_);

        if (currentFrame_ >= 15)
            return true;
        
        return false;
    }

};



// Examples of systems

struct PhysicsSystem
{
    PhysicsSystem(ECS::Registry<Components> &reg_) :
        m_physical{reg_.makeQuery<ComponentTransform, ComponentPhysical>()}
    {
        
    }

    void update()
    {
        m_physical.apply<ComponentTransform, ComponentPhysical>(updateCon);
    }

    static void updateCon(const ECS::EntityIndex &idx_, ComponentTransform &transform_, ComponentPhysical &physical_)
    {
        physical_.m_velocity.y += physical_.m_gravity;
        transform_.m_pos += physical_.m_velocity;
    }

    ECS::Query<Components> m_physical;
};

struct InputSystem
{
    InputSystem(ECS::Registry<Components> &reg_) :
        m_inq{reg_.makeQuery<ComponentPlayerInput>()}
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

        m_inq.update();

        m_inq.revapply<ComponentPlayerInput>([l, r](const auto &idx_, ComponentPlayerInput &inp_)
        {
            inp_.m_inL = l;
            inp_.m_inR = r;
        });

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

    ECS::Query<Components> m_inq;
};

struct RenderSystem
{
    RenderSystem(ECS::Registry<Components> &reg_) :
        m_renderable{reg_.makeQuery<ComponentTransform>()}
    {
        
    }

    void update()
    {
        std::cout << "=== RENDERING ===\n";
        m_renderable.update();
        m_renderable.applyview([](const ECS::EntityIndex &idx_, ECS::CheapEntityView<Components> view_)
        {
            iteratePrint<1>(view_);

            std::cout << std::endl;
        });
    }

    template<int CurrentType>
    static void iteratePrint(ECS::CheapEntityView<Components> &view_)
    {
        if (view_.contains<typename Components::template GetById<CurrentType>>())
        {
            std::cout << view_.get<typename Components::template GetById<CurrentType>>() << ", ";
        }

        if constexpr (CurrentType < Components::MaxID)
            iteratePrint<CurrentType + 1>(view_);
    }


    ECS::Query<Components> m_renderable;
};

struct PlayerStateSystem
{
public:
    PlayerStateSystem(ECS::Registry<Components> &reg_) :
        m_query{reg_.makeQuery<ComponentPlayerInput, StateMachine>()},
        m_reg(reg_)
    {
    }

    void initAll()
    {
        m_query.update();
        m_query.revapply<StateMachine>([](const auto &idx_, StateMachine &smc_)
        {
            smc_.addState(std::unique_ptr<StateIdle<PlayerStates>>(new StateIdle<PlayerStates>(PlayerStates::IDLE, {PlayerStates::NONE, {PlayerStates::RUN}})));
            smc_.addState(std::unique_ptr<StateRun>(new StateRun(5.0f)));
            smc_.setInitialState(PlayerStates::IDLE);
        });
    }

    void updateAll()
    {
        m_query.update();
        m_query.revapply<StateMachine>([&reg = this->m_reg](const auto &idx_, StateMachine &smc_)
        {
            auto view = reg[idx_.m_archetypeId].makeView<ComponentTransform, ComponentPhysical, ComponentPlayerInput>(idx_.m_entityId);
            smc_.update(view, 0);
        });
    }

private:
    ECS::Query<Components> m_query;
    ECS::Registry<Components> &m_reg;
};

struct MobStateSystem
{
public:
    MobStateSystem(ECS::Registry<Components> &reg_) :
        m_query{reg_.makeQuery<ComponentTransform, ComponentMobNavigation, StateMachine>()},
        m_reg(reg_)
    {
    }

    void initAll()
    {
        m_query.update();
        m_query.revapply<StateMachine>([](const auto &idx_, StateMachine &smc_)
        {
            auto tmproam = std::unique_ptr<StateMobMetaRoam>(new StateMobMetaRoam());
            tmproam->addState(std::unique_ptr<StateMobNavigation>(new StateMobNavigation(MobStates::WALK, 2.0f, {MobStates::NONE, {MobStates::IDLE}})));
            tmproam->addState(std::unique_ptr<StateIdle<MobStates>>(new StateIdle<MobStates>(MobStates::IDLE, {MobStates::NONE, {MobStates::WALK, MobStates::RUN}})));
            tmproam->setInitialState(MobStates::IDLE);

            auto tmpchase = std::unique_ptr<StateMobMetaChase>(new StateMobMetaChase());
            tmpchase->addState(std::unique_ptr<StateMobNavigation>(new StateMobNavigation(MobStates::RUN, 5.0f, {MobStates::NONE, {MobStates::IDLE, MobStates::WALK}})));
            tmpchase->setInitialState(MobStates::RUN);

            smc_.addState(std::move(tmproam));
            smc_.addState(std::move(tmpchase));

            smc_.setInitialState(MobStates::META_ROAM);
        });
    }

    void updateAll()
    {
        m_query.update();
        m_query.revapply<StateMachine>([&reg = this->m_reg](const auto &idx_, StateMachine &smc_)
        {
            auto view = reg[idx_.m_archetypeId].makeView<ComponentTransform, ComponentPhysical, ComponentMobNavigation>(idx_.m_entityId);
            smc_.update(view, 0);
        });
    }

private:
    ECS::Query<Components> m_query;
    ECS::Registry<Components> &m_reg;
};

void doNothing()
{
    std::cout << "It all lies\n";
}

int main(int argc, char* args[])
{
    MultiCallImpl<void, 1> mc(doNothing, [](){std::cout << "Do nothing 2\n";});

    mc();

    /*ECS::Registry<Components> reg;
    reg.createEntity<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput, StateMachine>(
        ComponentTransform(), ComponentPhysical({2.3f, 39.9f, 10.0f, 15.0f}, 9.8f), ComponentCharacter("Nameless1", 1), ComponentPlayerInput());
    reg.createEntity<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentMobNavigation, StateMachine>(
        ComponentPhysical({2.3f, 39.9f, 10.0f, 15.0f}, 0.1f), ComponentCharacter("Scary Skeleton", 3) );

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
    }*/
    /*ECS::Registry<Components> reg;
    reg.createEntity<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentPlayerInput, StateMachine<Components>>(
        ComponentTransform(), ComponentPhysical({2.3f, 39.9f, 10.0f, 15.0f}, 9.8f), ComponentCharacter("Nameless1", 1), ComponentPlayerInput(), StateMachine<Components>());
    reg.createEntity<ComponentTransform, ComponentPhysical, ComponentCharacter, ComponentMobNavigation, StateMachine<Components>(
        ComponentTransform(), ComponentPhysical({2.3f, 39.9f, 10.0f, 15.0f}, 0.1f), ComponentCharacter("Scary Skeleton", 3), ComponentMobNavigation(), StateMachine<Components>());

    auto qr = reg.getQuery<ComponentTransform, ComponentCharacter>();
    auto view = qr.makeView<ComponentTransform, ComponentCharacter>();
    utils::dumpType(std::cout, typeid(view).name());

    PhysicsSystem phys(reg);

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
    }*/

    /*std::cout << "Hello\n";

    utils::dumpType(std::cout, typeid(Components).name());
    std::cout << std::endl;

    ECS::Registry<Components> reg;
    auto todel = reg.createEntity<ComponentTransform, ComponentMobNavigation, ComponentCharacter>(ComponentTransform({32.1f, 59.2f}, {1.0f, 1.0f}), ComponentCharacter("MyChar", 2));
    reg.createEntity<ComponentTransform, ComponentMobNavigation, ComponentCharacter>();
    reg.createEntity<ComponentTransform>(ComponentTransform({-1.1f, 178.2f}, {1.0f, 1.0f}));
    auto toextend = reg.createEntity<ComponentTransform>();
    reg.emplaceComponents(toextend, ComponentMobNavigation(), ComponentPhysical({1, 2, 3, 4}, 9.8f));
    reg.removeEntity(todel);
    reg.removeComponents<ComponentMobNavigation>(todel);


    for (int i = 0; i < 30; ++i)
    {
        if (i % 3 == 0)
        {
            reg.createEntity<ComponentTransform, ComponentMobNavigation, ComponentCharacter>(ComponentTransform({30.1f + i, 59.2f}, {1.0f, 1.0f}), ComponentCharacter("MyChar", i));
        }
        else if (i % 3 == 1)
        {
            auto created = reg.createEntity<ComponentTransform, ComponentPhysical>(ComponentTransform({30.1f + i, 59.2f}, {1.0f, 1.0f}));
            reg.getComponent<ComponentPhysical>(created).m_velocity = {1.0f, 0.0f};
        }
        else
        {
            auto created = reg.createEntity<ComponentTransform, ComponentPhysical>(ComponentTransform({30.1f + i, 59.2f}, {1.0f, 1.0f}), ComponentPhysical({1, 2, 3, 4}, 9.8f));
        }
    }

    reg.dumpAll();

    std::cout << "Iterating backward\n";
    auto view = reg.makeQuery<ComponentTransform>();
    view.revapply<ComponentTransform, ComponentPhysical>([](const ECS::EntityIndex &idx_, ComponentTransform& trans_, const ComponentPhysical &phys_){
        trans_.m_pos += phys_.m_velocity;
        std::cout << trans_ << std::endl;
    });

    RenderSystem sys(reg);
    std::cout << "Rendering ===\n";
    sys.update();
    //reg.dumpAll();

    auto tpview = reg.makeQuery<ComponentTransform, ComponentPhysical>();
    tpview.revapply<ComponentTransform, ComponentPhysical>([&reg](const auto& idx_, ComponentTransform&, ComponentPhysical&)
    {
        reg.emplaceComponents(idx_, ComponentCharacter("sample", 1));
    });
    std::cout << "Finished\n"; 

    sys.update();*/

}
