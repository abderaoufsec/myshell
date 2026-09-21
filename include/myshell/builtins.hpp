#pragma once

#include "myshell/types.hpp"

namespace myshell
{

/**
 * @brief Check if a command is a built-in shell command
 * 
 * Built-in commands are commands that the shell executes directly
 * rather than spawning a new process. They are necessary for commands
 * that need to modify the shell's own state.
 * 
 * Key concepts:
 * - Built-ins run in the shell process itself (no fork/exec needed)
 * - They can modify shell state (current directory, environment variables)
 * - They are faster than external commands (no process creation overhead)
 * - Some commands MUST be built-ins (like cd) to work correctly
 * 
 * Why cd must be a built-in:
 * - If cd ran in a child process (fork/exec), only the child would change directory
 * - The shell process would remain in the original directory
 * - Therefore cd must execute in the shell process itself
 * 
 * Built-in commands implemented:
 * - cd: Change directory
 * - pwd: Print working directory
 * - echo: Print arguments
 * - export: Set environment variable
 * - unset: Remove environment variable
 * - exit: Exit the shell
 * - help: Display help information
 * 
 * @param command The command to check
 * @return true if the command is a built-in, false otherwise
 */
bool is_builtin(const Command& command);

/**
 * @brief Execute a built-in command
 * 
 * This function implements all the built-in shell commands.
 * Built-ins run directly in the shell process without creating a child process.
 * 
 * Key concepts:
 * - Built-ins modify shell state or provide shell functionality
 * - Return true on success, false on failure
 * - Some built-ins (like exit) terminate the entire shell process
 * - Environment changes affect the shell and all future child processes
 * 
 * Execution examples:
 * - "cd /tmp": Changes shell's current directory using chdir()
 * - "pwd": Prints current directory using filesystem API
 * - "export PATH=/bin": Sets environment variable using setenv()
 * - "unset PATH": Removes environment variable using unsetenv()
 * - "exit": Calls std::exit() to terminate the shell
 * 
 * @param command The built-in command to execute
 * @return true if command succeeded, false if it failed
 */
bool execute_builtin(const Command& command);

} // namespace myshell