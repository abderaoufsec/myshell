#pragma once

#include "myshell/types.hpp"

namespace myshell
{

class Executor
{
public:
    int execute(const Pipeline& pipeline) const;

private:
    int execute_command(const Pipeline& pipeline) const;
};

} // namespace myshell