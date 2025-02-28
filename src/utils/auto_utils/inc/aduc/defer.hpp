#pragma once

#include <functional>

namespace aduc
{
/**
 * @brief Unconditionally execute a function at scope exit.
 *
 */
class Defer
{
    std::function<void()> scope_exit_fn;

public:
    explicit Defer(std::function<void()>&& fn) : scope_exit_fn(std::move(fn))
    {
    }

    ~Defer()
    {
        scope_exit_fn();
    }
};

} // namespace aduc
