#pragma once

#include "myshell/executor.hpp"
#include "myshell/parser.hpp"

namespace myshell
{

class Shell
{
public:
    void run();

private:
    Parser parser_;
    Executor executor_;
};

} // namespace myshell