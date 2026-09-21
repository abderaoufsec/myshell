#pragma once

#include "myshell/types.hpp"

namespace myshell
{

/**
 * @brief Executes parsed shell commands
 * 
 * This class handles the execution of commands parsed by the Parser.
 * It distinguishes between built-in commands (run in shell process)
 * and external commands (run in child processes via fork/exec).
 * 
 * Key concepts:
 * - Built-in execution: Commands run in shell process (cd, pwd, echo, etc.)
 * - External execution: Commands run in child processes (ls, cat, grep, etc.)
 * - Process creation: fork() creates child, exec() replaces child, wait() waits for child
 * - I/O redirection: Manipulating file descriptors (stdin/stdout/stderr)
 * - Pipes: Inter-process communication between commands
 * - Job control: Managing foreground/background processes
 * 
 * Execution flow:
 * 1. Receive Pipeline from Parser
 * 2. Check if built-in or external command
 * 3. For built-ins: Execute directly in shell process
 * 4. For external: fork() child process, exec() command, wait() for completion
 * 5. Handle I/O redirections and pipes
 * 6. Return exit status
 * 
 * Platform differences:
 * - Linux: Uses fork/exec/wait (proper POSIX process management)
 * - Windows: Uses system() (educational compromise, less secure)
 * 
 * Example for "ls -la":
 * 1. Check if "ls" is built-in (no)
 * 2. fork() child process
 * 3. In child: execvp("ls", ["ls", "-la", nullptr])
 * 4. In parent: waitpid() for child to complete
 * 5. Return child's exit status
 * 
 * Example for "cd /tmp":
 * 1. Check if "cd" is built-in (yes)
 * 2. Execute chdir("/tmp") in shell process
 * 3. Return success/failure status
 */
class Executor
{
public:
    /**
     * @brief Execute a parsed pipeline
     * 
     * This is the main entry point for command execution. It determines
     * whether to execute a single command or a pipeline of commands.
     * 
     * @param pipeline The parsed pipeline to execute
     * @return Exit status (0 on success, non-zero on failure)
     */
    int execute(const Pipeline& pipeline) const;

private:
    /**
     * @brief Execute a single command (built-in or external)
     * 
     * This function handles execution of individual commands, including
     * built-in commands (run in shell process) and external commands
     * (run in child processes).
     * 
     * @param pipeline The pipeline containing a single command
     * @return Exit status (0 on success, non-zero on failure)
     */
    int execute_command(const Pipeline& pipeline) const;
};

} // namespace myshell