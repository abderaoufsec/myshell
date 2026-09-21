#pragma once

#include "myshell/executor.hpp"
#include "myshell/parser.hpp"

namespace myshell
{

/**
 * @brief Main shell class implementing the REPL interface
 * 
 * This class provides the top-level shell interface that users interact with.
 * It implements the Read-Eval-Print Loop (REPL) pattern common to all shells.
 * 
 * Key concepts:
 * - REPL: Read-Eval-Print Loop cycle
 * - Read: Get user input from stdin
 * - Eval: Parse and execute the command
 * - Print: Display output (handled by executed commands)
 * - Loop: Repeat until user exits
 * 
 * Architecture:
 * - Shell coordinates between Parser and Executor
 * - Parser: Converts text to structured commands
 * - Executor: Runs the commands (built-ins or external)
 * - Signal handling: Ignores Ctrl+C to protect shell process
 * 
 * Example interaction:
 * 1. Shell prints "myshell$ "
 * 2. User types "echo hello"
 * 3. Parser converts to Command structure
 * 4. Executor runs the echo command
 * 5. Output "hello" appears
 * 6. Loop repeats, prompt appears again
 */
class Shell
{
public:
    /**
     * @brief Run the shell's REPL loop
     * 
     * This method starts the interactive shell and continues until:
     * - User types "exit" command
     * - End of input (Ctrl+D) is detected
     * - Fatal error occurs
     * 
     * The loop:
     * 1. Print prompt "myshell$ "
     * 2. Read user input line
     * 3. Parse input into Pipeline structure
     * 4. Execute the pipeline
     * 5. Handle errors gracefully
     * 6. Repeat
     */
    void run();

private:
    Parser parser_;    // Parses user input into structured commands
    Executor executor_; // Executes parsed commands (built-ins or external)
};

} // namespace myshell