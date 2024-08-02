#include "StateMachine.h"

void StateMachine::addState(std::unique_ptr<GenericState> &&state_)
{
    m_stateIds[state_->m_stateId] = m_states.size();
    state_->setParent(this);
    m_states.push_back(std::move(state_));
}

void StateMachine::switchCurrentState(ECS::EntityView &owner_, GenericState *state_)
{
    m_currentState->leave(owner_, state_->m_stateId);
    state_->enter(owner_, m_currentState->m_stateId);
    m_currentState = state_;
    m_framesInState = 0;
}

bool StateMachine::update(ECS::EntityView &owner_, uint32_t currentFrame_)
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

bool StateMachine::attemptTransition(ECS::EntityView &owner_)
{
    auto currentStateId = m_currentState->m_stateId;
    auto &trans = owner_.get<ComponentTransform>(1);
    for (auto &el : m_states)
    {
        if (!el->transitionableFrom(currentStateId))
            continue;

        auto res = el->isPossible(owner_);
        if (res != ORIENTATION::UNSPECIFIED)
        {
            trans.m_orientation = res;
            switchCurrentState(owner_, el.get());
            return true;
        }
    }

    return false;
}

std::string StateMachine::getName() const
{
    return std::string("root") + " -> " + m_currentState->getName(m_framesInState);
}

std::ostream &operator<<(std::ostream &os_, const StateMachine &rhs_)
{
    os_ << rhs_.getName();
    return os_;
}

void GenericState::setParent(StateMachine *parent_)
{
    m_parent = parent_;
}

void GenericState::enter(ECS::EntityView &owner_, CharState from_)
{
    std::cout << "Switched to " << m_stateName << std::endl;
}

void GenericState::leave(ECS::EntityView &owner_, CharState to_)
{
}

bool GenericState::update(ECS::EntityView &owner_, uint32_t currentFrame_)
{
    return true;
}

ORIENTATION GenericState::isPossible(ECS::EntityView &owner_) const
{
    return owner_.get<ComponentTransform>(1).m_orientation;
}

std::string GenericState::getName(uint32_t framesInState_) const
{
    return m_stateName + " (" + std::to_string(framesInState_) + ")";
}

std::string NodeState::getName(uint32_t framesInState_) const
{
    return std::string(GenericState::m_stateName) + " (" + std::to_string(framesInState_) + ") -> " + StateMachine::m_currentState->getName(StateMachine::m_framesInState);
}

bool NodeState::update(ECS::EntityView &owner_, uint32_t currentFrame_)
{
    GenericState::update(owner_, currentFrame_);
    return StateMachine::update(owner_, currentFrame_);
}
