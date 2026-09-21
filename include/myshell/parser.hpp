#pragma once

#include "myshell/types.hpp"

#include <string>

namespace myshell
{

/**
 * @brief Parses shell input into structured command representations
 * 
 * This class handles the lexical analysis and parsing of shell command lines,
 * converting raw text into structured Pipeline objects that the executor can understand.
 * 
 * Key concepts:
 * - Tokenization: Breaking input into meaningful tokens (words, operators)
 * - Lexical analysis: Identifying special characters and their meanings
 * - Parsing: Building structured data from tokens
 * - Shell grammar: Rules for valid shell command syntax
 * 
 * Features handled:
 * - Whitespace separation
 * - Quoted strings (single ' and double ")
 * - Escape sequences (\)
 * - Environment variable expansion ($VAR, ${VAR})
 * - Redirection operators (>, >>, <)
 * - Pipe operator (|)
 * - Background execution (&)
 * 
 * Example parsing:
 * Input: "echo 'hello world' > output.txt &"
 * 1. Tokenize: ["echo", "'hello world'", ">", "output.txt", "&"]
 * 2. Process quotes: ["echo", "hello world", ">", "output.txt", "&"]
 * 3. Build structure:
 *    - commands: [{program: "echo", arguments: ["hello world"]}]
 *    - redirections: [{type: Output, filename: "output.txt"}]
 *    - background: true
 * 
 * The parser is deliberately simple (whitespace-based) but handles
 * the most common shell syntax needed for educational purposes.
 */
class Parser
{
public:
    /**
     * @brief Parse a shell command line into a structured Pipeline
     * 
     * This is the main parsing function that converts raw user input
     * into a structured Pipeline object containing commands, redirections,
     * and execution mode.
     * 
     * Processing steps:
     * 1. Tokenize input into words and operators
     * 2. Process escape sequences
     * 3. Strip quotes from tokens
     * 4. Expand environment variables
     * 5. Build Pipeline structure
     * 
     * @param input The raw command line string from user
     * @return Pipeline Structured representation of the command
     * @throws std::runtime_error if input is malformed
     */
    Pipeline parse(const std::string& input) const;
};

} // namespace myshell