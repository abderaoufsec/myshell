#include "myshell/shell.hpp"

#include <iostream>
#include <string>

namespace myshell
{

void Shell::run()
{
    while (true)
    {
        std::cout << "myshell$ ";
        std::cout.flush();

        std::string input;

        if (!std::getline(std::cin, input))
        {
            std::cout << '\n';
            break;
        }

        if (input.empty())
        {
            continue;
        }

        try
        {
            const Pipeline pipeline = parser_.parse(input);

            executor_.execute(pipeline);
        }
        catch (const std::exception& error)
        {
            std::cerr << "myshell: "
                      << error.what()
                      << '\n';
        }
    }
}

} // namespace myshell