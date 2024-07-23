#ifndef STATE_MARKER_H_
#define STATE_MARKER_H_

#include <iostream>
#include <array>
#include <chrono>

using StateHolder_t = uint64_t;
constexpr inline int STATE_HOLDER_SIZE = sizeof(StateHolder_t) * 8;

// Hold count_ bool values set to false by default
// For some reason works notably faster than just a regular std::vector<bool> with compiler optimization
// which should do the same thing under the hood
template<typename ENUM_TYPE>
class StateMarker
{
public:
    inline StateMarker(ENUM_TYPE lastElemP1_, const std::vector<ENUM_TYPE> &trueFields_)  :
        m_stateMarks(static_cast<int>(lastElemP1_) / STATE_HOLDER_SIZE + 1 , 0)
    {
        for (const auto &el: trueFields_)
        {
            auto casted = static_cast<int>(el);
            auto arrid = casted / STATE_HOLDER_SIZE;
            auto bitid = casted % STATE_HOLDER_SIZE;
            m_stateMarks[arrid] = m_stateMarks[arrid] | ((StateHolder_t)1 << bitid);
        }
    }

    void toggleMark(ENUM_TYPE id_)
    {
        auto casted = static_cast<int>(id_);
        auto arrid = casted / STATE_HOLDER_SIZE;
        auto bitid = casted % STATE_HOLDER_SIZE;
        m_stateMarks[arrid] = m_stateMarks[arrid] ^ ((StateHolder_t)1 << bitid);
    }

    inline bool operator[](const ENUM_TYPE &id_) const
    {
        auto casted = static_cast<int>(id_);

        auto arrid = casted / STATE_HOLDER_SIZE;
        auto bitid = casted % STATE_HOLDER_SIZE;
        return ((m_stateMarks[arrid] >> bitid) & (StateHolder_t)1);
    }

private:
    std::vector<StateHolder_t> m_stateMarks;
};



#endif