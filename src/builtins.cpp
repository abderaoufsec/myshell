#include "myshell/builtins.hpp"
#include "myshell/posix_compat.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace myshell
{

/**
 * @brief Check if a command is a built-in shell command
 * 
 * Built-in commands are commands that the shell executes directly
 * rather than spawning a new process. They are needed because they
 * modify the shell's own state (like current directory or environment).
 * 
 * Key concepts:
 * - Built-ins run in the shell process itself (no fork/exec)
 * - They can modify shell state (cd, export, unset)
 * - They are faster than external commands (no process creation)
 * - Some are necessary (cd can't work in child process)
 * 
 * Why cd must be a built-in:
 * - If cd ran in a child process, only the child would change directory
 * - The shell would remain in the original directory
 * - Therefore cd must execute in the shell process itself
 */
bool is_builtin(const Command& command)
{
    // Check if the command program name matches any built-in
    return command.program == "cd"      // Change directory
        || command.program == "pwd"     // Print working directory
        || command.program == "echo"    // Print arguments
        || command.program == "export"  // Set environment variable
        || command.program == "unset"  // Remove environment variable
        || command.program == "exit"    // Exit the shell
        || command.program == "help";   // Display help
}

/**
 * @brief Execute a built-in command
 * 
 * This function implements all the built-in shell commands.
 * Built-ins run directly in the shell process without creating a child.
 * 
 * Key concepts:
 * - Each built-in modifies shell state or provides shell functionality
 * - Return true on success, false on failure
 * - Some built-ins (like exit) terminate the entire shell process
 * - Environment changes affect the shell and all future child processes
 * 
 * @param command The command to execute (program name + arguments)
 * @return true if command succeeded, false if it failed
 */
bool execute_builtin(const Command& command)
{
    // EXIT COMMAND
    // Terminates the shell process immediately
    if (command.program == "exit")
    {
        std::exit(0); // Exit with success status
    }

    // CD COMMAND (Change Directory)
    // Changes the shell's current working directory
    if (command.program == "cd")
    {
        const char* directory = nullptr;

        // If no argument provided, go to HOME directory
        if (command.arguments.empty())
        {
            directory = std::getenv("HOME");
        }
        // Otherwise, use the provided directory path
        else
        {
            directory = command.arguments[0].c_str();
        }

        // Check if HOME is set when no argument provided
        if (directory == nullptr)
        {
            std::cerr << "myshell: cd: HOME not set\n";
            return false;
        }

        // Actually change the directory using POSIX chdir()
        // This changes the directory for the shell process itself
        if (chdir(directory) != 0)
        {
            perror("myshell: cd"); // Print error message with system error description
            return false;
        }

        return true;
    }

    // PWD COMMAND (Print Working Directory)
    // Displays the current directory path
    if (command.program == "pwd")
    {
        try
        {
            // Use C++ filesystem API to get current path
            std::cout << std::filesystem::current_path().string()
                      << '\n';
        }
        catch (const std::filesystem::filesystem_error& error)
        {
            // Handle filesystem errors (e.g., directory deleted)
            std::cerr << "myshell: pwd: "
                      << error.what()
                      << '\n';

            return false;
        }

        return true;
    }

    // ECHO COMMAND
    // Prints its arguments to stdout
    if (command.program == "echo")
    {
        // Print each argument separated by spaces
        for (std::size_t i = 0; i < command.arguments.size(); ++i)
        {
            if (i > 0)
            {
                std::cout << ' '; // Space between arguments
            }

            std::cout << command.arguments[i];
        }

        std::cout << '\n'; // End with newline

        return true;
    }

    // EXPORT COMMAND
    // Sets environment variables that will be inherited by child processes
    if (command.program == "export")
    {
        // export with no arguments is valid (does nothing)
        if (command.arguments.empty())
        {
            return true;
        }

        // Process each NAME=value argument
        for (const auto& argument : command.arguments)
        {
            // Find the '=' separator
            const auto position = argument.find('=');

            // Validate format: must have '='
            if (position == std::string::npos)
            {
                std::cerr << "myshell: export: invalid argument\n";
                continue;
            }

            // Split into name and value
            const std::string name = argument.substr(0, position);
            const std::string value = argument.substr(position + 1);

            // Set the environment variable
            // Third parameter (1) means overwrite if already exists
            if (::setenv(name.c_str(), value.c_str(), 1) != 0)
            {
                perror("myshell: export");
                return false;
            }
        }

        return true;
    }

    // UNSET COMMAND
    // Removes environment variables
    if (command.program == "unset")
    {
        // Remove each specified environment variable
        for (const auto& name : command.arguments)
        {
            if (::unsetenv(name.c_str()) != 0)
            {
                perror("myshell: unset");
                return false;
            }
        }

        return true;
    }

    // HELP COMMAND
    // Displays information about built-in commands
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

    // If we get here, the command wasn't recognized as a built-in
    return false;
}

} // namespace myshell