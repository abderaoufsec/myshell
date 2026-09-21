#pragma once

#include <string>
#include <vector>

namespace myshell
{

/**
 * @brief Represents a single shell command
 * 
 * This structure holds the parsed representation of a single command,
 * including the program name and its arguments.
 * 
 * Key concepts:
 * - program: The executable name (e.g., "ls", "echo", "grep")
 * - arguments: Command-line arguments (e.g., "-la", "/tmp")
 * - argv[0] convention: program is both the executable and first argument
 * 
 * Example: "ls -la /tmp"
 * - program = "ls"
 * - arguments = ["-la", "/tmp"]
 * 
 * When passed to execvp(), this becomes:
 * - argv[0] = "ls"
 * - argv[1] = "-la"
 * - argv[2] = "/tmp"
 * - argv[3] = nullptr
 */
struct Command
{
    std::string program;              // The command/program name (e.g., "ls")
    std::vector<std::string> arguments; // Command arguments (e.g., ["-la", "/tmp"])
};

/**
 * @brief Represents an I/O redirection specification
 * 
 * This structure holds information about input/output redirections,
 * including the type of redirection and the target file.
 * 
 * Key concepts:
 * - Input redirection (<): Read from file instead of stdin
 * - Output redirection (>): Write to file (overwrite existing)
 * - Append redirection (>>): Write to file (preserve existing)
 * - File descriptors: stdin=0, stdout=1, stderr=2
 * 
 * Example: "echo hello > output.txt"
 * - type = Output
 * - filename = "output.txt"
 * 
 * Example: "cat < input.txt"
 * - type = Input
 * - filename = "input.txt"
 */
struct Redirection
{
    // Enumeration of redirection types
    enum class Type
    {
        Input,   // < - Read from file (redirect stdin)
        Output,  // > - Write to file (overwrite, redirect stdout)
        Append   // >> - Write to file (append, redirect stdout)
    };

    Type type;           // The type of redirection
    std::string filename; // The target file for redirection
};

/**
 * @brief Represents a complete command pipeline
 * 
 * This structure holds the complete parsed representation of a command line,
 * including all commands, redirections, and execution mode.
 * 
 * Key concepts:
 * - Pipeline: Multiple commands connected by pipes (|)
 * - Commands: Vector of commands in execution order
 * - Redirections: I/O redirections for the pipeline
 * - Background: Execute without waiting for completion (&)
 * 
 * Example: "ls | grep txt > results.txt &"
 * - commands = [{program:"ls", arguments:[]}, {program:"grep", arguments:["txt"]}]
 * - redirections = [{type:Output, filename:"results.txt"}]
 * - background = true
 * 
 * Example: "echo hello"
 * - commands = [{program:"echo", arguments:["hello"]}]
 * - redirections = []
 * - background = false
 */
struct Pipeline
{
    std::vector<Command> commands;      // All commands in the pipeline (in order)
    std::vector<Redirection> redirections; // I/O redirections for the pipeline
    bool background = false;            // Execute in background (&)
};

} // namespace myshell