#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_
#include "yaECS.hpp"
#include "TypeManip.hpp"
#include "Vector2.h"
#include "StateMarker.hpp"
#include "ExampleComponents.h"

using CharState = int;

class GenericState;

class StateMachine
{
public:
    StateMachine() = default;
    StateMachine(const StateMachine &) = delete;
    StateMachine(StateMachine &&) = default;
    StateMachine &operator=(const StateMachine &) = delete;
    StateMachine &operator=(StateMachine &&) = default;

    void addState(std::unique_ptr<GenericState> &&state_);
    void switchCurrentState(ECS::EntityView &owner_, GenericState *state_);
    bool attemptTransition(ECS::EntityView &owner_);

    virtual bool update(ECS::EntityView &owner_, uint32_t currentFrame_);
    virtual std::string getName() const;

    template<typename PLAYER_STATE_T>
    void switchCurrentState(ECS::EntityView &owner_, PLAYER_STATE_T stateId_)
    {
        switchCurrentState(owner_, m_states[m_stateIds[static_cast<CharState>(stateId_)]].get());
    }
    
    template<typename PLAYER_STATE_T>
    inline void setInitialState(PLAYER_STATE_T state_)
    {
        auto initstate = static_cast<CharState>(state_);
        m_framesInState = 0;
        m_currentState = m_states[m_stateIds[initstate]].get();
    }

    std::vector<std::unique_ptr<GenericState>> m_states;
    std::unordered_map<CharState, int> m_stateIds;

    GenericState *m_currentState = nullptr;
    uint32_t m_framesInState = 0;
};

std::ostream &operator<<(std::ostream &os_, const StateMachine &rhs_);

class GenericState
{
public:
    template<typename PLAYER_STATE_T>
    GenericState(PLAYER_STATE_T stateId_, const std::string &stateName_, StateMarker &&transitionableFrom_) :
        m_stateId(static_cast<CharState>(stateId_)),
        m_stateName(stateName_),
        m_transitionableFrom(std::move(transitionableFrom_))
    {}

    void setParent(StateMachine *parent_);

    virtual void enter(ECS::EntityView &owner_, CharState from_);
    virtual void leave(ECS::EntityView &owner_, CharState to_);
    virtual bool update(ECS::EntityView &owner_, uint32_t currentFrame_);
    virtual ORIENTATION isPossible(ECS::EntityView &owner_) const;
    virtual std::string getName(uint32_t framesInState_) const;

    template<typename PLAYER_STATE_T>
    bool transitionableFrom(PLAYER_STATE_T state_) const
    {
        return m_transitionableFrom[state_];
    }

    const CharState m_stateId;

protected:
    const StateMarker m_transitionableFrom;
    std::string m_stateName;
    StateMachine *m_parent = nullptr;
};

/*
    Probably not very useful because you most likely will want to use different base classes
*/
class NodeState: public StateMachine, public GenericState
{
public:
    template<typename PLAYER_STATE_T>
    NodeState(PLAYER_STATE_T stateId_, const std::string &stateName_, StateMarker &&transitionableFrom_) :
        GenericState(stateId_, stateName_, std::move(transitionableFrom_))
    {}

    virtual std::string getName(uint32_t framesInState_) const override;
    virtual bool update(ECS::EntityView &owner_, uint32_t currentFrame_) override;
};

#endif