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

std::string expand_variable_token(const std::string& token)
{
    std::string output;
    for (std::size_t i = 0; i < token.size(); ++i)
    {
        if (token[i] == '$' && i + 1 < token.size())
        {
            if (token[i + 1] == '{')
            {
                const std::size_t end = token.find('}', i + 2);
                if (end != std::string::npos)
                {
                    const std::string name = token.substr(i + 2, end - i - 2);
                    const char* value = std::getenv(name.c_str());
                    if (value != nullptr)
                    {
                        output += value;
                    }
                    i = end;
                    continue;
                }
            }

            if (std::isalnum(static_cast<unsigned char>(token[i + 1])) ||
                token[i + 1] == '_')
            {
                std::size_t start = i + 1;
                while (start < token.size() &&
                       (std::isalnum(static_cast<unsigned char>(token[start])) ||
                        token[start] == '_'))
                {
                    ++start;
                }

                const std::string name = token.substr(i + 1, start - i - 1);
                const char* value = std::getenv(name.c_str());
                if (value != nullptr)
                {
                    output += value;
                }

                i = start - 1;
                continue;
            }
        }

        output.push_back(token[i]);
    }

    return output;
}

std::string decode_escaped_token(const std::string& token)
{
    std::string out;
    bool escape = false;

    for (char c : token)
    {
        if (escape)
        {
            out.push_back(c);
            escape = false;
        }
        else if (c == '\\')
        {
            escape = true;
        }
        else
        {
            out.push_back(c);
        }
    }

    return out;
}

std::string strip_quotes(const std::string& token)
{
    if (token.size() >= 2)
    {
        if ((token.front() == '\'' && token.back() == '\'') ||
            (token.front() == '"' && token.back() == '"'))
        {
            return token.substr(1, token.size() - 2);
        }
    }

    return token;
}

std::vector<std::string> tokenize(const std::string& input)
{
    std::vector<std::string> tokens;
    std::string token;
    bool in_single = false;
    bool in_double = false;
    bool escaped = false;

    auto flush_token = [&]() {
        if (!token.empty())
        {
            tokens.push_back(token);
            token.clear();
        }
    };

    for (std::size_t i = 0; i < input.size(); ++i)
    {
        const char c = input[i];

        if (escaped)
        {
            token.push_back(c);
            escaped = false;
            continue;
        }

        if (c == '\\' && !in_single)
        {
            escaped = true;
            continue;
        }

        if (c == '\'' && !in_double)
        {
            in_single = !in_single;
            continue;
        }

        if (c == '"' && !in_single)
        {
            in_double = !in_double;
            continue;
        }

        if (std::isspace(static_cast<unsigned char>(c)) && !in_single && !in_double)
        {
            flush_token();
            continue;
        }

        if (!in_single && !in_double && c == '|' && token.empty())
        {
            flush_token();
            tokens.push_back("|");
            continue;
        }

        if (!in_single && !in_double && c == '>' && token.empty())
        {
            flush_token();
            if (i + 1 < input.size() && input[i + 1] == '>')
            {
                tokens.push_back(">>");
                ++i;
            }
            else
            {
                tokens.push_back(">");
            }
            continue;
        }

        if (!in_single && !in_double && c == '<' && token.empty())
        {
            flush_token();
            tokens.push_back("<");
            continue;
        }

        if (!in_single && !in_double && c == '&' && token.empty())
        {
            flush_token();
            tokens.push_back("&");
            continue;
        }

        token.push_back(c);
    }

    if (escaped)
    {
        token.push_back(' ');
    }

    if (!token.empty())
    {
        tokens.push_back(token);
    }

    return tokens;
}

} // namespace

Pipeline Parser::parse(const std::string& input) const
{
    Pipeline pipeline;
    Command command;

    const std::vector<std::string> words = tokenize(input);

    for (std::size_t i = 0; i < words.size(); ++i)
    {
        std::string word = words[i];
        word = strip_quotes(decode_escaped_token(word));
        word = expand_variable_token(word);

        if (word == "|")
        {
            if (command.program.empty())
            {
                throw std::runtime_error("invalid pipeline");
            }

            pipeline.commands.push_back(command);
            command = Command{};
            continue;
        }

        if (word == ">" || word == ">>" || word == "<")
        {
            if (i + 1 >= words.size())
            {
                throw std::runtime_error("missing redirection filename");
            }

            const std::string filename =
                strip_quotes(decode_escaped_token(words[++i]));

            pipeline.redirections.push_back(
                {
                    word == ">" ? Redirection::Type::Output :
                    word == ">>" ? Redirection::Type::Append :
                    Redirection::Type::Input,
                    expand_variable_token(filename)
                });
            continue;
        }

        if (word == "&")
        {
            pipeline.background = true;
            continue;
        }

        if (command.program.empty())
        {
            command.program = word;
        }
        else
        {
            command.arguments.push_back(word);
        }
    }

    if (!command.program.empty())
    {
        pipeline.commands.push_back(command);
    }

    return pipeline;
}

} // namespace myshell