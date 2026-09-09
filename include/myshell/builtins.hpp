#pragma once

#include "myshell/types.hpp"

namespace myshell
{

bool is_builtin(const Command& command);

bool execute_builtin(const Command& command);

} // namespace myshell