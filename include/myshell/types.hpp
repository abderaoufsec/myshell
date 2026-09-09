#pragma once

#include <string>
#include <vector>

namespace myshell
{

struct Command
{
    std::string program;
    std::vector<std::string> arguments;
};

struct Redirection
{
    enum class Type
    {
        Input,
        Output,
        Append
    };

    Type type;
    std::string filename;
};

struct Pipeline
{
    std::vector<Command> commands;
    std::vector<Redirection> redirections;
    bool background = false;
};

} // namespace myshell