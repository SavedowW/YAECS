#ifndef MULTIPLE_CALL_H_
#define MULTIPLE_CALL_H_
#include <array>
#include <functional>

template<typename Ret, typename... Args>
class MultiCall
{
public:
    virtual void operator()(Args... args_) = 0;
};

template<typename Ret, size_t Size, typename... Args>
class MultiCallImpl : public MultiCall<Ret, Args...>
{
public:
    template<typename... Fs>
    MultiCallImpl(const Fs&... fs_) :
        m_funcs{fs_...}
    {
    }

    virtual void operator()(Args... args_) override
    {
        for (auto &el : m_funcs)
            el(args_...);
    }

private:
    std::array<std::function<Ret(Args...)>, Size> m_funcs;
};

#endif