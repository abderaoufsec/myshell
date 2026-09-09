#include "myshell/executor.hpp"

#include "myshell/builtins.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>


namespace myshell
{

int Executor::execute(const Pipeline& pipeline) const
{
    if (pipeline.commands.empty())
    {
        return 0;
    }

    if (pipeline.commands.size() == 1 &&
        is_builtin(pipeline.commands[0]) &&
        !pipeline.background)
    {
        return execute_command(pipeline);
    }

    return 0;
}

int Executor::execute_command(const Pipeline& pipeline) const
{
    const Command& command = pipeline.commands[0];

if (is_builtin(command))
{
    return execute_builtin(command) ? 0 : 1;
}
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("myshell: fork");
        return 1;
    }

    if (pid == 0)
    {
        // --- Redirections placed inside child process ---
        for (const auto& redirection : pipeline.redirections)
        {
            if (redirection.type == Redirection::Type::Output)
            {
                int fd = open(
                    redirection.filename.c_str(),
                    O_WRONLY | O_CREAT | O_TRUNC,
                    0644
                );

                if (fd == -1)
                {
                    perror("myshell: open");
                    _exit(1); // Use _exit in child process instead of return
                }

                if (dup2(fd, STDOUT_FILENO) == -1)
                {
                    perror("myshell: dup2");
                    close(fd);
                    _exit(1); // Use _exit in child process instead of return
                }

                close(fd);
            }
        }

        std::vector<char*> argv;

        argv.push_back(const_cast<char*>(
            command.program.c_str()));

        for (const auto& argument : command.arguments)
        {
            argv.push_back(
                const_cast<char*>(argument.c_str()));
        }

        argv.push_back(nullptr);

        execvp(command.program.c_str(), argv.data());

        std::cerr << "myshell: " << command.program
                  << ": " << std::strerror(errno)
                  << '\n';

        _exit(127);
    }

    int status = 0;

    if (waitpid(pid, &status, 0) < 0)
    {
        perror("myshell: waitpid");
        return 1;
    }

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }

    return 1;
}

} // namespace myshell