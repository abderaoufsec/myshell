#include "myshell/shell.hpp"

/**
 * @brief Entry point for the myshell program
 * 
 * This is the main function that initializes and runs the shell.
 * It's deliberately simple - the real work happens in the Shell class.
 * 
 * Key concepts:
 * - Shell: The main REPL (Read-Eval-Print Loop) interface
 * - This function just creates a Shell object and starts it
 * - Returns 0 on successful exit
 */
int main()
{
    // Create a Shell instance - this encapsulates all shell functionality
    myshell::Shell shell;
    
    // Run the shell's REPL loop - this will continue until the user exits
    // The run() method handles:
    // - Printing the prompt
    // - Reading user input
    // - Parsing commands
    // - Executing commands
    // - Looping until exit
    shell.run();

    // Return 0 to indicate successful program termination
    return 0;
}