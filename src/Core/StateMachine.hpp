#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_
#include "TypeManip.hpp"
#include "Vector2.h"
#include "StateMarker.hpp"

template<typename Owner_t, typename PLAYER_STATE_T>
class GenericState;

template<typename Owner_t, typename PLAYER_STATE_T>
class StateMachine
{
protected:
    using OwnedState = GenericState<Owner_t, PLAYER_STATE_T>;

public:
    StateMachine() = default;
    StateMachine(const StateMachine &) = delete;
    StateMachine(StateMachine &&) = default;
    StateMachine &operator=(const StateMachine &) = delete;
    StateMachine &operator=(StateMachine &&) = default;

    inline void addState(std::unique_ptr<OwnedState> &&state_)
    {
        m_stateIds[state_->m_stateId] = m_states.size();
        state_->setParent(this);
        m_states.push_back(std::move(state_));
    }

    inline void setInitialState(PLAYER_STATE_T state_)
    {
        m_framesInState = 0;
        m_currentState = m_states[m_stateIds[state_]].get();
    }

    void switchCurrentState(Owner_t &owner_, OwnedState *state_)
    {
        m_currentState->leave(owner_, state_->m_stateId);
        state_->enter(owner_, m_currentState->m_stateId);
        m_currentState = state_;
        m_framesInState = 0;
    }

    void switchCurrentState(Owner_t &owner_, PLAYER_STATE_T stateId_)
    {
        switchCurrentState(owner_, m_states[m_stateIds[stateId_]].get());
    }

    inline virtual bool update(Owner_t &owner_, uint32_t currentFrame_)
    {
        if (m_currentState->update(owner_, m_framesInState))
        {
            if (!attemptTransition(owner_))
            {
                m_framesInState++;
            }
            else
                return true;
        }
        else
            m_framesInState++;

        return false;
    }

    bool attemptTransition(Owner_t &owner_)
    {
        auto currentStateId = m_currentState->m_stateId;
        for (auto &el : m_states)
        {
            if (!el->transitionableFrom(currentStateId))
                continue;

            auto res = el->isPossible(owner_);
            if (res != ORIENTATION::UNSPECIFIED)
            {
                auto &trans = std::get<ComponentTransform&>(owner_);
                trans.m_orientation = res;
                switchCurrentState(owner_, el.get());
                return true;
            }
        }

        return false;
    }

    virtual std::string getName() const
    {
        return std::string("root") + " -> " + m_currentState->getName(m_framesInState);
    }

    std::vector<std::unique_ptr<OwnedState>> m_states;
    std::unordered_map<PLAYER_STATE_T, int> m_stateIds;

    OwnedState *m_currentState = nullptr;
    uint32_t m_framesInState = 0;
};

template<typename Owner_t, typename PLAYER_STATE_T>
std::ostream &operator<<(std::ostream &os_, const StateMachine<Owner_t, PLAYER_STATE_T> &rhs_)
{
    os_ << rhs_.getName();
    return os_;
}

template<typename Owner_t, typename PLAYER_STATE_T>
class GenericState
{
protected:
    using ParentSM = StateMachine<Owner_t, PLAYER_STATE_T>;

public:
    GenericState(PLAYER_STATE_T stateId_, const std::string &stateName_, StateMarker<PLAYER_STATE_T> &&transitionableFrom_) :
        m_stateId(stateId_),
        m_stateName(stateName_),
        m_transitionableFrom(std::move(transitionableFrom_))
    {}

    void setParent(ParentSM *parent_)
    {
        m_parent = parent_;
    }

    inline virtual void enter(Owner_t &owner_, PLAYER_STATE_T from_)
    {
        std::cout << "Switched to " << m_stateName << std::endl;
    }

    inline virtual void leave(Owner_t &owner_, PLAYER_STATE_T to_)
    {
    }

    inline virtual bool update(Owner_t &owner_, uint32_t currentFrame_)
    {
        return true;
    }

    inline virtual ORIENTATION isPossible(Owner_t &owner_) const
    {
        return std::get<ComponentTransform&>(owner_).m_orientation;
    }

    virtual std::string getName(uint32_t framesInState_) const
    {
        return m_stateName + " (" + std::to_string(framesInState_) + ")";
    }

    bool transitionableFrom(PLAYER_STATE_T state_) const
    {
        return m_transitionableFrom[state_];
    }

    const PLAYER_STATE_T m_stateId;

protected:
    const StateMarker<PLAYER_STATE_T> m_transitionableFrom;
    std::string m_stateName;
    ParentSM *m_parent = nullptr;
};

template<typename Owner_t, typename PLAYER_STATE_T>
class NodeState: public StateMachine<Owner_t, PLAYER_STATE_T>, public GenericState<Owner_t, PLAYER_STATE_T>
{
public:
    NodeState(PLAYER_STATE_T stateId_, const std::string &stateName_, StateMarker<PLAYER_STATE_T> &&transitionableFrom_) :
        GenericState<Owner_t, PLAYER_STATE_T>(stateId_, stateName_, std::move(transitionableFrom_))
    {}

    virtual std::string getName(uint32_t framesInState_) const override
    {
        return std::string(GenericState<Owner_t, PLAYER_STATE_T>::m_stateName) + " (" + std::to_string(framesInState_) + ") -> " + StateMachine<Owner_t, PLAYER_STATE_T>::m_currentState->getName(StateMachine<Owner_t, PLAYER_STATE_T>::m_framesInState);
    }

    inline virtual bool update(Owner_t &owner_, uint32_t currentFrame_) override
    {
        GenericState<Owner_t, PLAYER_STATE_T>::update(owner_, currentFrame_);
        return StateMachine<Owner_t, PLAYER_STATE_T>::update(owner_, currentFrame_);
    }


private:

};

#endif