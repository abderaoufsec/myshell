#include "myshell/builtins.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <unistd.h>

namespace myshell
{

bool is_builtin(const Command& command)
{
    return command.program == "cd"
        || command.program == "pwd"
        || command.program == "echo"
        || command.program == "export"
        || command.program == "unset"
        || command.program == "exit"
        || command.program == "help";
}

bool execute_builtin(const Command& command)
{
    if (command.program == "exit")
    {
        std::exit(0);
    }

    if (command.program == "cd")
    {
        const char* directory = nullptr;

        if (command.arguments.empty())
        {
            directory = std::getenv("HOME");
        }
        else
        {
            directory = command.arguments[0].c_str();
        }

        if (directory == nullptr)
        {
            std::cerr << "myshell: cd: HOME not set\n";
            return false;
        }

        if (chdir(directory) != 0)
        {
            perror("myshell: cd");
            return false;
        }

        return true;
    }

    if (command.program == "pwd")
    {
        try
        {
            std::cout << std::filesystem::current_path().string()
                      << '\n';
        }
        catch (const std::filesystem::filesystem_error& error)
        {
            std::cerr << "myshell: pwd: "
                      << error.what()
                      << '\n';

            return false;
        }

        return true;
    }

    if (command.program == "echo")
    {
        for (std::size_t i = 0; i < command.arguments.size(); ++i)
        {
            if (i > 0)
            {
                std::cout << ' ';
            }

            std::cout << command.arguments[i];
        }

        std::cout << '\n';

        return true;
    }

    if (command.program == "export")
    {
        if (command.arguments.empty())
        {
            return true;
        }

        for (const auto& argument : command.arguments)
        {
            const auto position = argument.find('=');

            if (position == std::string::npos)
            {
                std::cerr << "myshell: export: invalid argument\n";
                continue;
            }

            const std::string name = argument.substr(0, position);
            const std::string value = argument.substr(position + 1);

            if (setenv(name.c_str(), value.c_str(), 1) != 0)
            {
                perror("myshell: export");
                return false;
            }
        }

        return true;
    }

    if (command.program == "unset")
    {
        for (const auto& name : command.arguments)
        {
            if (unsetenv(name.c_str()) != 0)
            {
                perror("myshell: unset");
                return false;
            }
        }

        return true;
    }

    if (command.program == "help")
    {
        std::cout
            << "myshell built-ins:\n"
            << "  cd [directory]\n"
            << "  pwd\n"
            << "  echo [arguments...]\n"
            << "  export NAME=value\n"
            << "  unset NAME\n"
            << "  exit\n"
            << "  help\n";

        return true;
    }

    return false;
}

} // namespace myshell