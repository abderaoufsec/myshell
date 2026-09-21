#include "myshell/shell.hpp"
#include "myshell/posix_compat.hpp"

#include <iostream>
#include <string>

namespace myshell
{

/**
 * @brief Main REPL (Read-Eval-Print Loop) for the shell
 * 
 * This method implements the core shell interaction loop:
 * 1. Print prompt
 * 2. Read user input
 * 3. Parse the input into commands
 * 4. Execute the commands
 * 5. Repeat until exit
 * 
 * Key concepts:
 * - REPL: Read-Eval-Print Loop pattern
 * - Signal handling: Ignore Ctrl+C so shell doesn't exit
 * - EOF handling: Detect when input stream ends (Ctrl+D)
 * - Exception handling: Gracefully handle parsing/execution errors
 */
void Shell::run()
{
    // Set up signal handlers
    // SIGINT = Ctrl+C, SIGQUIT = Ctrl+\
    // We ignore these so the shell itself doesn't exit when user presses them
    // Only the foreground child process should receive these signals
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);

    // Main REPL loop - runs forever until break
    while (true)
    {
        // Step 1: PRINT - Display the prompt
        std::cout << "myshell$ ";
        std::cout.flush(); // Ensure prompt appears immediately (no buffering)

        // Step 2: READ - Get user input
        std::string input;

        // getline reads a line of text from stdin
        // Returns false on EOF (end of file) or error
        if (!std::getline(std::cin, input))
        {
            // User pressed Ctrl+D (EOF) or input stream closed
            std::cout << '\n'; // Print newline for clean exit
            break; // Exit the loop and terminate shell
        }

        // Skip empty lines (user just pressed Enter)
        if (input.empty())
        {
            continue;
        }

        // Step 3: EVAL - Parse and execute the command
        try
        {
            // Parse the input string into a structured Pipeline object
            // The parser tokenizes the input and identifies:
            // - Commands and their arguments
            // - Redirections (>, >>, <)
            // - Pipes (|)
            // - Background execution (&)
            const Pipeline pipeline = parser_.parse(input);

            // Execute the parsed pipeline
            // The executor handles:
            // - Built-in commands (cd, pwd, echo, etc.)
            // - External commands (ls, cat, etc.)
            // - Process creation (fork/exec)
            // - I/O redirection
            // - Pipes
            // - Background processes
            executor_.execute(pipeline);
        }
        // Step 4: ERROR HANDLING - Catch and report any errors
        catch (const std::exception& error)
        {
            // Something went wrong in parsing or execution
            // Print the error message but continue running the shell
            // This ensures a single bad command doesn't crash the entire shell
            std::cerr << "myshell: "
                      << error.what()
                      << '\n';
        }
        // Loop continues - print prompt again for next command
    }
}

} // namespace myshell