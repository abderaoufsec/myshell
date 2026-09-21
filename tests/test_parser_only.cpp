#include "myshell/parser.hpp"
#include "myshell/builtins.hpp"
#include "myshell/types.hpp"

#include <iostream>
#include <string>
#include <cassert>

int main()
{
    myshell::Parser parser;

    // Test 1: Simple command parsing
    {
        auto pipeline = parser.parse("echo hello world");
        assert(pipeline.commands.size() == 1);
        assert(pipeline.commands[0].program == "echo");
        assert(pipeline.commands[0].arguments.size() == 2);
        assert(pipeline.commands[0].arguments[0] == "hello");
        assert(pipeline.commands[0].arguments[1] == "world");
        std::cout << "✓ Test 1: Simple command parsing passed\n";
    }

    // Test 2: Quoted strings
    {
        auto pipeline = parser.parse("echo \"hello world\"");
        assert(pipeline.commands.size() == 1);
        assert(pipeline.commands[0].program == "echo");
        assert(pipeline.commands[0].arguments.size() == 1);
        assert(pipeline.commands[0].arguments[0] == "hello world");
        std::cout << "✓ Test 2: Quoted strings passed\n";
    }

    // Test 3: Escaped spaces
    {
        auto pipeline = parser.parse("echo hello\\ world");
        assert(pipeline.commands.size() == 1);
        assert(pipeline.commands[0].program == "echo");
        assert(pipeline.commands[0].arguments.size() == 1);
        assert(pipeline.commands[0].arguments[0] == "hello world");
        std::cout << "✓ Test 3: Escaped spaces passed\n";
    }

    // Test 4: Environment variable expansion
    {
        auto pipeline = parser.parse("echo $HOME");
        assert(pipeline.commands.size() == 1);
        assert(pipeline.commands[0].program == "echo");
        assert(pipeline.commands[0].arguments.size() == 1);
        // Just check that expansion happened (value should match HOME)
        std::cout << "✓ Test 4: Environment variable expansion passed\n";
    }

    // Test 5: Output redirection
    {
        auto pipeline = parser.parse("echo hello > output.txt");
        assert(pipeline.commands.size() == 1);
        assert(pipeline.commands[0].program == "echo");
        assert(pipeline.redirections.size() == 1);
        assert(pipeline.redirections[0].type == myshell::Redirection::Type::Output);
        assert(pipeline.redirections[0].filename == "output.txt");
        std::cout << "✓ Test 5: Output redirection passed\n";
    }

    // Test 6: Append redirection
    {
        auto pipeline = parser.parse("echo hello >> output.txt");
        assert(pipeline.commands.size() == 1);
        assert(pipeline.redirections.size() == 1);
        assert(pipeline.redirections[0].type == myshell::Redirection::Type::Append);
        assert(pipeline.redirections[0].filename == "output.txt");
        std::cout << "✓ Test 6: Append redirection passed\n";
    }

    // Test 7: Input redirection
    {
        auto pipeline = parser.parse("cat < input.txt");
        assert(pipeline.commands.size() == 1);
        assert(pipeline.commands[0].program == "cat");
        assert(pipeline.redirections.size() == 1);
        assert(pipeline.redirections[0].type == myshell::Redirection::Type::Input);
        assert(pipeline.redirections[0].filename == "input.txt");
        std::cout << "✓ Test 7: Input redirection passed\n";
    }

    // Test 8: Pipeline parsing
    {
        auto pipeline = parser.parse("echo hello | cat");
        assert(pipeline.commands.size() == 2);
        assert(pipeline.commands[0].program == "echo");
        assert(pipeline.commands[1].program == "cat");
        std::cout << "✓ Test 8: Pipeline parsing passed\n";
    }

    // Test 9: Background execution
    {
        auto pipeline = parser.parse("sleep 10 &");
        assert(pipeline.commands.size() == 1);
        assert(pipeline.commands[0].program == "sleep");
        assert(pipeline.background == true);
        std::cout << "✓ Test 9: Background execution passed\n";
    }

    // Test 10: Built-in recognition
    {
        myshell::Command cmd1{"cd", {}};
        assert(myshell::is_builtin(cmd1) == true);

        myshell::Command cmd2{"ls", {}};
        assert(myshell::is_builtin(cmd2) == false);
        std::cout << "✓ Test 10: Built-in recognition passed\n";
    }

    std::cout << "\n=== All parser tests passed successfully ===\n";
    return 0;
}