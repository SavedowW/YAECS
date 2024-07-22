#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_
#include "TypeManip.hpp"
#include "Vector2.h"

template<typename Owner_t>
class GenericState;

template<typename Owner_t>
class StateMachine
{
protected:
    using OwnedState = GenericState<Owner_t>;

public:
    StateMachine() = default;
    StateMachine(const StateMachine &) = delete;
    StateMachine(StateMachine &&) = default;
    StateMachine &operator=(const StateMachine &) = delete;
    StateMachine &operator=(StateMachine &&) = default;

    template<typename T>
    inline void addState(std::unique_ptr<T> &&state_)
    {
        m_stateIds[TypeManip::TypeHash::getHash<T>()] = m_states.size();
        state_->setParent(this);
        m_states.push_back(std::move(state_));
    }

    inline void setInitialState(int state_)
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

    void switchCurrentState(Owner_t &owner_, int stateId_)
    {
        switchCurrentState(owner_, m_states[m_stateIds[stateId_]].get());
    }

    inline virtual void update(Owner_t &owner_, uint32_t currentFrame_)
    {
        if (m_currentState->update(owner_, currentFrame_))
        {
            if (!attemptTransition(owner_))
                m_framesInState++;
        }
        else
            m_framesInState++;
    }

    bool attemptTransition(Owner_t &owner_)
    {
        for (auto &el : m_states)
        {
            if (m_currentState == el.get())
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
        return std::string("root") + " -> " + m_currentState->getName();
    }

    std::vector<std::unique_ptr<OwnedState>> m_states;
    std::unordered_map<int, int> m_stateIds;

    OwnedState *m_currentState = nullptr;
    uint32_t m_framesInState = 0;
};

template<typename Owner_t>
std::ostream &operator<<(std::ostream &os_, const StateMachine<Owner_t> &rhs_)
{
    os_ << rhs_.getName() << " (" << rhs_.m_framesInState << ")";
    return os_;
}

template<typename Owner_t>
class GenericState
{
public:
    GenericState(int stateId_, const std::string &stateName_) :
        m_stateId(stateId_),
        m_stateName(stateName_)
    {}

    void setParent(StateMachine<Owner_t> *parent_)
    {
        m_parent = parent_;
    }

    inline virtual void enter(Owner_t &owner_, int from_)
    {
        std::cout << "Switched to " << m_stateName << std::endl;
    }

    inline virtual void leave(Owner_t &owner_, int to_)
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

    virtual std::string getName() const
    {
        return m_stateName;
    }

    const int m_stateId;

protected:
    std::string m_stateName;
    StateMachine<Owner_t> *m_parent = nullptr;
};

#endif