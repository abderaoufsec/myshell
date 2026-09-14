#include "myshell/executor.hpp"

#include "myshell/builtins.hpp"
#include "myshell/jobs.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>
#include <fcntl.h>

namespace myshell
{
namespace
{

bool apply_redirections_to_child(const Pipeline& pipeline)
{
    for (const auto& redirection : pipeline.redirections)
    {
        int fd = -1;

        if (redirection.type == Redirection::Type::Input)
        {
            fd = open(redirection.filename.c_str(), O_RDONLY);
            if (fd == -1)
            {
                perror("myshell: open");
                return false;
            }

            if (dup2(fd, STDIN_FILENO) == -1)
            {
                perror("myshell: dup2");
                close(fd);
                return false;
            }

            close(fd);
        }
        else if (redirection.type == Redirection::Type::Output)
        {
            fd = open(redirection.filename.c_str(),
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644);

            if (fd == -1)
            {
                perror("myshell: open");
                return false;
            }

            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                perror("myshell: dup2");
                close(fd);
                return false;
            }

            close(fd);
        }
        else if (redirection.type == Redirection::Type::Append)
        {
            fd = open(redirection.filename.c_str(),
                      O_WRONLY | O_CREAT | O_APPEND,
                      0644);

            if (fd == -1)
            {
                perror("myshell: open");
                return false;
            }

            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                perror("myshell: dup2");
                close(fd);
                return false;
            }

            close(fd);
        }
    }

    return true;
}

bool apply_redirections_to_parent(const Pipeline& pipeline,
                                  std::vector<int>& saved_fds)
{
    saved_fds.push_back(dup(STDIN_FILENO));
    saved_fds.push_back(dup(STDOUT_FILENO));
    saved_fds.push_back(dup(STDERR_FILENO));

    for (const auto& redirection : pipeline.redirections)
    {
        int fd = -1;

        if (redirection.type == Redirection::Type::Input)
        {
            fd = open(redirection.filename.c_str(), O_RDONLY);
            if (fd == -1)
            {
                perror("myshell: open");
                return false;
            }

            if (dup2(fd, STDIN_FILENO) == -1)
            {
                perror("myshell: dup2");
                close(fd);
                return false;
            }

            close(fd);
        }
        else if (redirection.type == Redirection::Type::Output)
        {
            fd = open(redirection.filename.c_str(),
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644);

            if (fd == -1)
            {
                perror("myshell: open");
                return false;
            }

            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                perror("myshell: dup2");
                close(fd);
                return false;
            }

            close(fd);
        }
        else if (redirection.type == Redirection::Type::Append)
        {
            fd = open(redirection.filename.c_str(),
                      O_WRONLY | O_CREAT | O_APPEND,
                      0644);

            if (fd == -1)
            {
                perror("myshell: open");
                return false;
            }

            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                perror("myshell: dup2");
                close(fd);
                return false;
            }

            close(fd);
        }
    }

    return true;
}

void restore_parent_redirections(const std::vector<int>& saved_fds)
{
    if (saved_fds.size() >= 3)
    {
        dup2(saved_fds[0], STDIN_FILENO);
        dup2(saved_fds[1], STDOUT_FILENO);
        dup2(saved_fds[2], STDERR_FILENO);

        close(saved_fds[0]);
        close(saved_fds[1]);
        close(saved_fds[2]);
    }
}

void run_child_external(const Command& command)
{
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);

    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(command.program.c_str()));

    for (const auto& argument : command.arguments)
    {
        argv.push_back(const_cast<char*>(argument.c_str()));
    }

    argv.push_back(nullptr);

    execvp(command.program.c_str(), argv.data());

    std::cerr << "myshell: " << command.program
              << ": " << std::strerror(errno)
              << '\n';

    _exit(127);
}

void run_child_builtin(const Command& command)
{
    const bool ok = execute_builtin(command);
    std::cout.flush();
    _exit(ok ? 0 : 1);
}

int execute_pipeline(const Pipeline& pipeline)
{
    const std::size_t command_count = pipeline.commands.size();
    const std::size_t pipe_count = command_count - 1;

    std::vector<int> pipe_fds;
    pipe_fds.reserve(pipe_count * 2);

    for (std::size_t i = 0; i < pipe_count; ++i)
    {
        int raw_pipe[2] = { -1, -1 };
        if (pipe(raw_pipe) == -1)
        {
            perror("myshell: pipe");
            return 1;
        }

        pipe_fds.push_back(raw_pipe[0]);
        pipe_fds.push_back(raw_pipe[1]);
    }

    std::vector<pid_t> child_pids;
    child_pids.reserve(command_count);
    pid_t first_child_pid = -1;

    for (std::size_t i = 0; i < command_count; ++i)
    {
        const Command& command = pipeline.commands[i];
        pid_t pid = fork();

        if (pid < 0)
        {
            perror("myshell: fork");
            for (int fd : pipe_fds)
            {
                close(fd);
            }
            return 1;
        }

        if (pid == 0)
        {
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);

            if (i > 0)
            {
                const int read_end = pipe_fds[(i - 1) * 2];
                if (dup2(read_end, STDIN_FILENO) == -1)
                {
                    perror("myshell: dup2");
                    _exit(1);
                }
            }

            if (i < command_count - 1)
            {
                const int write_end = pipe_fds[i * 2 + 1];
                if (dup2(write_end, STDOUT_FILENO) == -1)
                {
                    perror("myshell: dup2");
                    _exit(1);
                }
            }

            for (int fd : pipe_fds)
            {
                close(fd);
            }

            if (i == 0)
            {
                for (const auto& redirection : pipeline.redirections)
                {
                    if (redirection.type == Redirection::Type::Input)
                    {
                        int fd = open(redirection.filename.c_str(), O_RDONLY);
                        if (fd == -1)
                        {
                            perror("myshell: open");
                            _exit(1);
                        }

                        if (dup2(fd, STDIN_FILENO) == -1)
                        {
                            perror("myshell: dup2");
                            close(fd);
                            _exit(1);
                        }

                        close(fd);
                    }
                }
            }

            if (i == command_count - 1)
            {
                for (const auto& redirection : pipeline.redirections)
                {
                    if (redirection.type == Redirection::Type::Output)
                    {
                        int fd = open(redirection.filename.c_str(),
                                      O_WRONLY | O_CREAT | O_TRUNC,
                                      0644);

                        if (fd == -1)
                        {
                            perror("myshell: open");
                            _exit(1);
                        }

                        if (dup2(fd, STDOUT_FILENO) == -1)
                        {
                            perror("myshell: dup2");
                            close(fd);
                            _exit(1);
                        }

                        close(fd);
                    }
                    else if (redirection.type == Redirection::Type::Append)
                    {
                        int fd = open(redirection.filename.c_str(),
                                      O_WRONLY | O_CREAT | O_APPEND,
                                      0644);

                        if (fd == -1)
                        {
                            perror("myshell: open");
                            _exit(1);
                        }

                        if (dup2(fd, STDOUT_FILENO) == -1)
                        {
                            perror("myshell: dup2");
                            close(fd);
                            _exit(1);
                        }

                        close(fd);
                    }
                }
            }

            if (is_builtin(command))
            {
                run_child_builtin(command);
            }

            run_child_external(command);
        }

        if (first_child_pid == -1)
        {
            first_child_pid = pid;
        }

        if (setpgid(pid, first_child_pid) != 0)
        {
            perror("myshell: setpgid");
        }

        child_pids.push_back(pid);
    }

    Job job;
    job.command = pipeline.commands.front().program;
    job.background = pipeline.background;
    job.pgid = first_child_pid;
    job.pids = child_pids;
    job.state = JobState::Running;
    JobManager::instance().add(job);

    for (int fd : pipe_fds)
    {
        close(fd);
    }

    if (pipeline.background)
    {
        return 0;
    }

    JobManager::instance().set_foreground_group(first_child_pid);

    int final_status = 0;
    for (pid_t pid : child_pids)
    {
        int status = 0;
        if (waitpid(pid, &status, 0) < 0)
        {
            perror("myshell: waitpid");
            final_status = 1;
            continue;
        }

        if (WIFEXITED(status))
        {
            const int code = WEXITSTATUS(status);
            if (code != 0)
            {
                final_status = code;
            }
        }
        else
        {
            final_status = 1;
        }

        JobManager::instance().mark_done(pid);
        JobManager::instance().remove(pid);
    }

    JobManager::instance().restore_foreground_group();

    return final_status;
}

} // namespace

int Executor::execute(const Pipeline& pipeline) const
{
    if (pipeline.commands.empty())
    {
        return 0;
    }

    if (pipeline.commands.size() == 1)
    {
        return execute_command(pipeline);
    }

    return execute_pipeline(pipeline);
}

int Executor::execute_command(const Pipeline& pipeline) const
{
    const Command& command = pipeline.commands[0];

    if (is_builtin(command))
    {
        if (pipeline.background)
        {
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("myshell: fork");
                return 1;
            }

            if (pid == 0)
            {
                if (!apply_redirections_to_child(pipeline))
                {
                    _exit(1);
                }

                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);

                const bool ok = execute_builtin(command);
                std::cout.flush();
                _exit(ok ? 0 : 1);
            }

            if (setpgid(pid, pid) != 0)
            {
                perror("myshell: setpgid");
            }

            Job job;
            job.command = command.program;
            job.background = true;
            job.pgid = pid;
            job.pids.push_back(pid);
            JobManager::instance().add(job);
            return 0;
        }

        if (pipeline.redirections.empty())
        {
            const bool ok = execute_builtin(command);
            std::cout.flush();
            return ok ? 0 : 1;
        }

        std::vector<int> saved_fds;
        if (!apply_redirections_to_parent(pipeline, saved_fds))
        {
            return 1;
        }

        const bool ok = execute_builtin(command);
        std::cout.flush();
        restore_parent_redirections(saved_fds);
        return ok ? 0 : 1;
    }

    pid_t pid = fork();

    if (pid < 0)
    {
        perror("myshell: fork");
        return 1;
    }

    if (pid == 0)
    {
        if (!apply_redirections_to_child(pipeline))
        {
            _exit(1);
        }

        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        run_child_external(command);
    }

    if (setpgid(pid, pid) != 0)
    {
        perror("myshell: setpgid");
    }

    Job job;
    job.command = command.program;
    job.background = pipeline.background;
    job.pgid = pid;
    job.pids.push_back(pid);
    job.state = JobState::Running;
    JobManager::instance().add(job);

    if (!pipeline.background)
    {
        JobManager::instance().set_foreground_group(pid);
    }

    int status = 0;
    if (!pipeline.background)
    {
        if (waitpid(pid, &status, 0) < 0)
        {
            perror("myshell: waitpid");
            return 1;
        }

        JobManager::instance().restore_foreground_group();
        JobManager::instance().mark_done(pid);
        JobManager::instance().remove(pid);

        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }

        return 1;
    }

    return 0;
}

} // namespace myshell