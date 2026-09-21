#include "myshell/parser.hpp"

#include <cctype>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

namespace myshell
{
namespace
{

/**
 * @brief Expand environment variables in a token
 * 
 * This function handles both forms of variable expansion:
 * - $VAR - Simple variable name
 * - ${VAR} - Braced variable name
 * 
 * Examples:
 * - "$HOME" -> "/home/user"
 * - "${PATH}" -> "/usr/bin:/bin"
 * - "prefix_$SUFFIX" -> "prefix_value"
 * 
 * Key concepts:
 * - Environment variables are inherited from parent process
 * - getenv() returns nullptr if variable doesn't exist
 * - We skip undefined variables (don't replace with empty string)
 */
std::string expand_variable_token(const std::string& token)
{
    std::string output; // Result string with variables expanded
    
    // Iterate through each character in the token
    for (std::size_t i = 0; i < token.size(); ++i)
    {
        // Check if this is a variable expansion starts with '$'
        if (token[i] == '$' && i + 1 < token.size())
        {
            // Handle ${VAR} syntax (braced form)
            if (token[i + 1] == '{')
            {
                // Find the closing brace
                const std::size_t end = token.find('}', i + 2);
                if (end != std::string::npos)
                {
                    // Extract variable name between ${ and }
                    const std::string name = token.substr(i + 2, end - i - 2);
                    
                    // Get the environment variable value
                    const char* value = std::getenv(name.c_str());
                    if (value != nullptr)
                    {
                        output += value; // Append the value if it exists
                    }
                    // If variable doesn't exist, we skip it (don't add anything)
                    
                    i = end; // Skip past the entire ${VAR} expression
                    continue;
                }
            }

            // Handle $VAR syntax (simple form)
            // Variable names can contain letters, numbers, and underscores
            if (std::isalnum(static_cast<unsigned char>(token[i + 1])) ||
                token[i + 1] == '_')
            {
                // Find the end of the variable name
                std::size_t start = i + 1;
                while (start < token.size() &&
                       (std::isalnum(static_cast<unsigned char>(token[start])) ||
                        token[start] == '_'))
                {
                    ++start;
                }

                // Extract the variable name
                const std::string name = token.substr(i + 1, start - i - 1);
                
                // Get the environment variable value
                const char* value = std::getenv(name.c_str());
                if (value != nullptr)
                {
                    output += value; // Append the value if it exists
                }

                i = start - 1; // Skip past the variable name
                continue;
            }
        }

        // If not a variable expansion, just copy the character
        output.push_back(token[i]);
    }

    return output;
}

/**
 * @brief Process escape sequences in a token
 * 
 * This function handles backslash escaping. When a backslash (\) is found,
 * the next character is taken literally (even if it's a special character).
 * 
 * Examples:
 * - "hello\ world" -> "hello world" (space is not a separator)
 * - "echo\ \$HOME" -> "echo $HOME" (dollar sign is not expanded)
 * - "path\\to\\file" -> "path\to\file" (backslashes preserved)
 * 
 * Key concepts:
 * - Escape sequences allow special characters to be treated as literals
 * - The backslash itself is not included in the output
 * - Escaping preserves the exact meaning of the next character
 */
std::string decode_escaped_token(const std::string& token)
{
    std::string out; // Output string with escapes processed
    bool escape = false; // Flag to track if we're in escape mode

    // Process each character in the token
    for (char c : token)
    {
        if (escape)
        {
            // Previous character was backslash, so this character is literal
            out.push_back(c);
            escape = false; // Reset escape flag
        }
        else if (c == '\\')
        {
            // This is an escape character - set flag for next iteration
            escape = true;
        }
        else
        {
            // Normal character - just copy it
            out.push_back(c);
        }
    }

    return out;
}

/**
 * @brief Remove surrounding quotes from a token
 * 
 * This function removes matching single or double quotes from the beginning
 * and end of a token. This is called after escape processing and before
 * variable expansion.
 * 
 * Examples:
 * - "'hello world'" -> "hello world"
 * - '"hello world"' -> "hello world"
 * - "'hello" -> "'hello" (no removal - quotes don't match)
 * - "hello" -> "hello" (no quotes to remove)
 * 
 * Key concepts:
 * - Quotes are used to group words with spaces into single arguments
 * - Both single (') and double (") quotes are supported
 * - Quotes must match at both ends to be removed
 * - Quoted content is treated as a single token (spaces don't separate)
 */
std::string strip_quotes(const std::string& token)
{
    // Only process if token has at least 2 characters (opening and closing quote)
    if (token.size() >= 2)
    {
        // Check if token starts and ends with matching quotes
        if ((token.front() == '\'' && token.back() == '\'') ||
            (token.front() == '"' && token.back() == '"'))
        {
            // Remove the quotes by taking substring from index 1 to size-2
            // This removes first and last character
            return token.substr(1, token.size() - 2);
        }
    }

    // No matching quotes found, return token unchanged
    return token;
}

/**
 * @brief Tokenize input string into shell tokens
 * 
 * This is the core lexer/tokenizer that breaks input into meaningful tokens.
 * It handles:
 * - Whitespace separation
 * - Single quotes (')
 * - Double quotes (")
 * - Escape sequences (\)
 * - Operators (|, >, >>, <, &)
 * 
 * Examples:
 * - "echo hello world" -> ["echo", "hello", "world"]
 * - "echo 'hello world'" -> ["echo", "'hello world'"]
 * - "ls | grep txt" -> ["ls", "|", "grep", "txt"]
 * - "echo hello > file.txt" -> ["echo", "hello", ">", "file.txt"]
 * 
 * Key concepts:
 * - State machine: tracks whether we're inside quotes, escape mode, etc.
 * - Operators are only recognized at token boundaries (when token is empty)
 * - Spaces inside quotes are preserved as part of the token
 * - Escape sequences are processed before quote stripping
 */
std::vector<std::string> tokenize(const std::string& input)
{
    std::vector<std::string> tokens; // Result: list of tokens
    std::string token; // Current token being built
    bool in_single = false; // Inside single quotes (')
    bool in_double = false; // Inside double quotes (")
    bool escaped = false; // After backslash (\)

    // Lambda function to save current token and start a new one
    auto flush_token = [&]() {
        if (!token.empty())
        {
            tokens.push_back(token);
            token.clear();
        }
    };

    // Process each character in the input string
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        const char c = input[i];

        // If we're in escape mode, just add the character literally
        if (escaped)
        {
            token.push_back(c);
            escaped = false; // Exit escape mode
            continue;
        }

        // Handle backslash - enter escape mode (except inside single quotes)
        if (c == '\\' && !in_single)
        {
            escaped = true;
            continue;
        }

        // Handle single quotes - toggle single-quote mode
        // Single quotes don't interact with double quotes
        if (c == '\'' && !in_double)
        {
            in_single = !in_single;
            continue;
        }

        // Handle double quotes - toggle double-quote mode
        // Double quotes don't interact with single quotes
        if (c == '"' && !in_single)
        {
            in_double = !in_double;
            continue;
        }

        // Handle whitespace - token separator (only outside quotes)
        if (std::isspace(static_cast<unsigned char>(c)) && !in_single && !in_double)
        {
            flush_token(); // Save current token and start new one
            continue;
        }

        // Handle pipe operator (|) - only at token boundaries
        if (!in_single && !in_double && c == '|' && token.empty())
        {
            flush_token();
            tokens.push_back("|"); // Add pipe as separate token
            continue;
        }

        // Handle output redirection (> and >>)
        if (!in_single && !in_double && c == '>' && token.empty())
        {
            flush_token();
            // Check if this is >> (append) or just > (overwrite)
            if (i + 1 < input.size() && input[i + 1] == '>')
            {
                tokens.push_back(">>");
                ++i; // Skip the second >
            }
            else
            {
                tokens.push_back(">");
            }
            continue;
        }

        // Handle input redirection (<)
        if (!in_single && !in_double && c == '<' && token.empty())
        {
            flush_token();
            tokens.push_back("<");
            continue;
        }

        // Handle background execution (&)
        if (!in_single && !in_double && c == '&' && token.empty())
        {
            flush_token();
            tokens.push_back("&");
            continue;
        }

        // Normal character - add to current token
        token.push_back(c);
    }

    // Handle trailing backslash (escape at end of input)
    if (escaped)
    {
        token.push_back(' '); // Replace with space
    }

    // Don't forget the last token if input didn't end with space
    if (!token.empty())
    {
        tokens.push_back(token);
    }

    return tokens;
}

} // namespace

/**
 * @brief Parse input string into a structured Pipeline object
 * 
 * This is the main parsing function that converts tokenized input into
 * a structured representation that the executor can understand.
 * 
 * Input: "echo hello > file.txt | cat"
 * Output: Pipeline with:
 *   - commands: [echo hello, cat]
 *   - redirections: [Output -> file.txt]
 *   - background: false
 * 
 * Key concepts:
 * - Pipeline: Represents a complete command line with all components
 * - Command: Individual program with its arguments
 * - Redirection: I/O redirection specifications
 * - Processing order: tokenize -> decode escapes -> strip quotes -> expand variables
 */
Pipeline Parser::parse(const std::string& input) const
{
    Pipeline pipeline; // Result: will contain all parsed information
    Command command;  // Current command being built

    // Step 1: Tokenize the input into words/tokens
    const std::vector<std::string> words = tokenize(input);

    // Step 2: Process each token and build the pipeline structure
    for (std::size_t i = 0; i < words.size(); ++i)
    {
        std::string word = words[i];
        
        // Step 3: Process the token in order:
        // 1. Decode escape sequences (e.g., "hello\ world" -> "hello world")
        // 2. Strip quotes (e.g., "'hello'" -> "hello")
        // 3. Expand variables (e.g., "$HOME" -> "/home/user")
        word = strip_quotes(decode_escaped_token(word));
        word = expand_variable_token(word);

        // Step 4: Handle special operators

        // PIPE OPERATOR (|)
        // This separates commands in a pipeline
        if (word == "|")
        {
            // Validate: can't have pipe without a command before it
            if (command.program.empty())
            {
                throw std::runtime_error("invalid pipeline");
            }

            // Save the current command to the pipeline
            pipeline.commands.push_back(command);
            
            // Start a new command for the next part of the pipeline
            command = Command{};
            continue;
        }

        // REDIRECTION OPERATORS (>, >>, <)
        if (word == ">" || word == ">>" || word == "<")
        {
            // Validate: need a filename after the operator
            if (i + 1 >= words.size())
            {
                throw std::runtime_error("missing redirection filename");
            }

            // Get the filename (next token) and process it
            const std::string filename =
                strip_quotes(decode_escaped_token(words[++i]));

            // Add redirection to pipeline
            pipeline.redirections.push_back(
                {
                    // Determine redirection type
                    word == ">" ? Redirection::Type::Output :
                    word == ">>" ? Redirection::Type::Append :
                    Redirection::Type::Input,
                    // Expand variables in filename (e.g., "echo > $HOME/file.txt")
                    expand_variable_token(filename)
                });
            continue;
        }

        // BACKGROUND EXECUTION (&)
        if (word == "&")
        {
            pipeline.background = true;
            continue;
        }

        // Step 5: Handle regular command parts

        // If this is the first word, it's the program name
        if (command.program.empty())
        {
            command.program = word;
        }
        // Otherwise, it's an argument to the program
        else
        {
            command.arguments.push_back(word);
        }
    }

    // Step 6: Don't forget the last command if there was no trailing pipe
    if (!command.program.empty())
    {
        pipeline.commands.push_back(command);
    }

    return pipeline;
}

} // namespace myshell