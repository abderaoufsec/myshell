#include "myshell/parser.hpp"

#include <sstream>
#include <stdexcept>

namespace myshell
{

Pipeline Parser::parse(const std::string& input) const
{
    Pipeline pipeline;

    std::istringstream stream(input);
    std::string word;

    Command command;

    while (stream >> word)
    {
        if (word == "|")
        {
            if (command.program.empty())
            {
                throw std::runtime_error("invalid pipeline");
            }

            pipeline.commands.push_back(command);
            command = Command{};
        }
        else if (word == ">")
        {
            std::string filename;

            if (!(stream >> filename))
            {
                throw std::runtime_error("missing output filename");
            }

            pipeline.redirections.push_back(
                {Redirection::Type::Output, filename});
        }
        else if (word == ">>")
        {
            std::string filename;

            if (!(stream >> filename))
            {
                throw std::runtime_error("missing output filename");
            }

            pipeline.redirections.push_back(
                {Redirection::Type::Append, filename});
        }
        else if (word == "<")
        {
            std::string filename;

            if (!(stream >> filename))
            {
                throw std::runtime_error("missing input filename");
            }

            pipeline.redirections.push_back(
                {Redirection::Type::Input, filename});
        }
        else if (word == "&")
        {
            pipeline.background = true;
        }
        else if (command.program.empty())
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