#pragma once

#include "myshell/types.hpp"

#include <string>

namespace myshell
{

class Parser
{
public:
    Pipeline parse(const std::string& input) const;
};

} // namespace myshell