#include "myshell/executor.hpp"

#include "myshell/builtins.hpp"
#include "myshell/jobs.hpp"
#include "myshell/posix_compat.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <vector>
#include <cstdlib>

namespace myshell
{
namespace
{

/**
 * @brief Apply I/O redirections to a child process
 * 
 * This function sets up file descriptor redirections for external commands
 * that run in child processes. It's called in the child after fork() but before exec().
 * 
 * Key concepts:
 * - File descriptors: 0=stdin, 1=stdout, 2=stderr
 * - open(): Opens a file and returns a file descriptor
 * - dup2(): Duplicates a file descriptor, making target refer to same file
 * - Redirections must happen in child to avoid affecting the shell itself
 * 
 * Example: "echo hello > file.txt"
 * 1. Open file.txt for writing
 * 2. Use dup2 to make stdout (fd 1) refer to file.txt
 * 3. Close the original file descriptor
 * 4. When exec runs, the program's stdout goes to file.txt
 * 
 * @param pipeline The pipeline containing redirection specifications
 * @return true if redirections succeeded, false on error
 */
bool apply_redirections_to_child(const Pipeline& pipeline)
{
    // Process each redirection in the pipeline
    for (const auto& redirection : pipeline.redirections)
    {
        int fd = -1; // File descriptor for the opened file

        // INPUT REDIRECTION (<)
        // Redirects stdin to read from a file
        if (redirection.type == Redirection::Type::Input)
        {
            // Open file for reading only
            fd = open(redirection.filename.c_str(), O_RDONLY);
            if (fd == -1)
            {
                perror("myshell: open");
                return false;
            }

            // Make stdin (fd 0) refer to the opened file
            // After this, reads from fd 0 will read from the file
            if (dup2(fd, STDIN_FILENO) == -1)
            {
                perror("myshell: dup2");
                close(fd);
                return false;
            }

            // Close the original file descriptor (no longer needed)
            close(fd);
        }
        // OUTPUT REDIRECTION (>)
        // Redirects stdout to write to a file (overwrites existing content)
        else if (redirection.type == Redirection::Type::Output)
        {
            // Open file for writing, create if doesn't exist, truncate if does
            // 0644 = file permissions (rw-r--r--)
            fd = open(redirection.filename.c_str(),
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644);

            if (fd == -1)
            {
                perror("myshell: open");
                return false;
            }

            // Make stdout (fd 1) refer to the opened file
            if (dup2(fd, STDOUT_FILENO) == -1)
            {
                perror("myshell: dup2");
                close(fd);
                return false;
            }

            close(fd);
        }
        // APPEND REDIRECTION (>>)
        // Redirects stdout to write to a file (preserves existing content)
        else if (redirection.type == Redirection::Type::Append)
        {
            // Open file for writing, create if doesn't exist, append if does
            fd = open(redirection.filename.c_str(),
                      O_WRONLY | O_CREAT | O_APPEND,
                      0644);

            if (fd == -1)
            {
                perror("myshell: open");
                return false;
            }

            // Make stdout (fd 1) refer to the opened file
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

/**
 * @brief Apply I/O redirections to the parent shell process
 * 
 * This function is used for built-in commands that need redirection.
 * Since built-ins run in the shell process itself, we need to:
 * 1. Save the current file descriptors
 * 2. Apply redirections
 * 3. Execute the built-in
 * 4. Restore the original file descriptors
 * 
 * This ensures the shell's stdin/stdout/stderr are not permanently affected.
 * 
 * Key concepts:
 * - Built-ins run in shell process, so redirection affects shell itself
 * - We must save and restore to avoid breaking the shell
 * - dup() creates a copy of a file descriptor (unlike dup2 which replaces)
 * 
 * Example: "echo hello > file.txt"
 * 1. Save current stdout (fd 1)
 * 2. Redirect stdout to file.txt
 * 3. Execute echo (writes to file.txt)
 * 4. Restore original stdout (shell prompt works normally again)
 * 
 * @param pipeline The pipeline containing redirection specifications
 * @param saved_fds Vector to store the saved file descriptors
 * @return true if redirections succeeded, false on error
 */
bool apply_redirections_to_parent(const Pipeline& pipeline,
                                  std::vector<int>& saved_fds)
{
    // Step 1: Save the current standard file descriptors
    // dup() creates a copy that we can restore later
    saved_fds.push_back(dup(STDIN_FILENO));   // Save stdin
    saved_fds.push_back(dup(STDOUT_FILENO));  // Save stdout
    saved_fds.push_back(dup(STDERR_FILENO));  // Save stderr

    // Step 2: Apply the redirections (same logic as child redirection)
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

/**
 * @brief Restore the parent shell's original file descriptors
 * 
 * This function restores the file descriptors that were saved before
 * applying redirections for built-in commands. This ensures the shell
 * continues to function normally after executing a built-in with redirection.
 * 
 * Key concepts:
 * - Must be called after built-in execution
 * - Restores stdin, stdout, stderr to their original state
 * - Closes the saved file descriptors (cleanup)
 * 
 * @param saved_fds Vector containing the saved file descriptors
 */
void restore_parent_redirections(const std::vector<int>& saved_fds)
{
    // Ensure we have all three saved descriptors
    if (saved_fds.size() >= 3)
    {
        // Restore each file descriptor to its original state
        dup2(saved_fds[0], STDIN_FILENO);   // Restore stdin
        dup2(saved_fds[1], STDOUT_FILENO);  // Restore stdout
        dup2(saved_fds[2], STDERR_FILENO);  // Restore stderr

        // Close the saved file descriptors (cleanup)
        close(saved_fds[0]);
        close(saved_fds[1]);
        close(saved_fds[2]);
    }
}

/**
 * @brief Execute an external command in a child process
 * 
 * This function is called in the child process after fork() to execute
 * an external program using execvp(). It replaces the child's process
 * image with the new program.
 * 
 * Key concepts:
 * - execvp() replaces the current process with a new program
 * - If execvp() returns, it failed (otherwise it never returns)
 * - argv must be null-terminated array of strings
 * - Signal handlers are reset to default for the child process
 * - _exit() is used instead of exit() to avoid flushing parent buffers
 * 
 * @param command The command to execute (program + arguments)
 */
void run_child_external(const Command& command)
{
    // Reset signal handlers to default for the child process
    // This allows Ctrl+C to work on the child process, not the shell
    signal(SIGINT, SIG_DFL);  // Default behavior for Ctrl+C
    signal(SIGQUIT, SIG_DFL); // Default behavior for Ctrl+\

    // Build the argv array for execvp()
    // execvp expects: argv[0]=program, argv[1..n]=arguments, argv[n+1]=nullptr
    std::vector<char*> argv;
    argv.push_back(const_cast<char*>(command.program.c_str())); // argv[0]

    for (const auto& argument : command.arguments)
    {
        argv.push_back(const_cast<char*>(argument.c_str())); // argv[1..n]
    }

    argv.push_back(nullptr); // argv[n+1] = nullptr (required by execvp)

    // Execute the program
    // execvp searches PATH if program doesn't contain '/'
    // This replaces the child process with the new program
    execvp(command.program.c_str(), argv.data());

    // If we get here, execvp() failed
    // Print error message and exit with error code
    std::cerr << "myshell: " << command.program
              << ": " << std::strerror(errno)
              << '\n';

    // Use _exit() instead of exit() to avoid flushing parent's stdio buffers
    // Exit code 127 is traditional for "command not found"
    _exit(127);
}

/**
 * @brief Execute a built-in command in a child process
 * 
 * This function is used when a built-in command needs to run in a
 * child process (e.g., for background execution with redirection).
 * 
 * Key concepts:
 * - Built-ins normally run in the shell process
 * - For background execution, they run in a child process
 * - Child process exits after executing the built-in
 * - Uses _exit() to avoid affecting parent process state
 * 
 * @param command The built-in command to execute
 */
void run_child_builtin(const Command& command)
{
    // Execute the built-in command
    const bool ok = execute_builtin(command);
    
    // Flush output to ensure it's written before exit
    std::cout.flush();
    
    // Exit with success (0) or failure (1) status
    // Use _exit() to avoid flushing parent's buffers
    _exit(ok ? 0 : 1);
}

/**
 * @brief Execute a pipeline of commands connected by pipes
 * 
 * This function handles complex pipelines like "cmd1 | cmd2 | cmd3".
 * It creates pipes between commands and manages multiple child processes.
 * 
 * Key concepts:
 * - Pipes connect stdout of one process to stdin of next
 * - pipe() creates a pair of file descriptors (read end, write end)
 * - Each command runs in its own child process
 * - All processes in a pipeline form a process group
 * - Parent waits for all children to complete (unless background)
 * 
 * Example: "ls | grep txt | wc -l"
 * 1. Create 2 pipes (ls->grep, grep->wc)
 * 2. Fork 3 child processes
 * 3. Connect them with pipes
 * 4. Execute each command
 * 5. Wait for all to complete
 * 
 * @param pipeline The pipeline containing multiple commands
 * @return Exit status of the pipeline (0 on success, non-zero on failure)
 */
int execute_pipeline(const Pipeline& pipeline)
{
#ifdef _WIN32
    // Windows implementation (educational compromise)
    // Windows doesn't support fork() and native pipes the same way
    // We use system() with Windows shell pipe syntax for educational purposes
    // This is less secure but allows the shell to work on Windows for learning
    
    std::string full_command;
    for (std::size_t i = 0; i < pipeline.commands.size(); ++i)
    {
        if (i > 0)
        {
            full_command += " | "; // Windows pipe syntax
        }
        full_command += pipeline.commands[i].program;
        for (const auto& arg : pipeline.commands[i].arguments)
        {
            full_command += " ";
            full_command += arg;
        }
    }

    // Handle redirections for Windows
    if (!pipeline.redirections.empty())
    {
        std::vector<int> saved_fds;
        if (!apply_redirections_to_parent(pipeline, saved_fds))
        {
            return 1;
        }

        int result = system(full_command.c_str());
        restore_parent_redirections(saved_fds);
        return result;
    }

    return system(full_command.c_str());
#else
    // Linux/POSIX implementation (proper fork/exec/pipe)
    
    // Calculate number of commands and pipes needed
    const std::size_t command_count = pipeline.commands.size();
    const std::size_t pipe_count = command_count - 1;

    // Create all pipes first
    // Each pipe has 2 file descriptors: read end and write end
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

        // Store both ends of the pipe
        pipe_fds.push_back(raw_pipe[0]); // Read end
        pipe_fds.push_back(raw_pipe[1]); // Write end
    }

    // Track all child process IDs
    std::vector<pid_t> child_pids;
    child_pids.reserve(command_count);
    pid_t first_child_pid = -1;

    // Fork a child process for each command
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
            // CHILD PROCESS CODE
            
            // Reset signal handlers for child
            signal(SIGINT, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);

            // Connect stdin to previous command's pipe (if not first command)
            if (i > 0)
            {
                const int read_end = pipe_fds[(i - 1) * 2];
                if (dup2(read_end, STDIN_FILENO) == -1)
                {
                    perror("myshell: dup2");
                    _exit(1);
                }
            }

            // Connect stdout to next command's pipe (if not last command)
            if (i < command_count - 1)
            {
                const int write_end = pipe_fds[i * 2 + 1];
                if (dup2(write_end, STDOUT_FILENO) == -1)
                {
                    perror("myshell: dup2");
                    _exit(1);
                }
            }

            // Close all pipe file descriptors in child
            // Child only needs the duplicated stdin/stdout
            for (int fd : pipe_fds)
            {
                close(fd);
            }

            // Handle input redirection for first command
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

            // Handle output redirection for last command
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

            // Execute the command (built-in or external)
            if (is_builtin(command))
            {
                run_child_builtin(command);
            }

            run_child_external(command);
        }

        // PARENT PROCESS CODE
        
        // Track the first child's PID as the process group leader
        if (first_child_pid == -1)
        {
            first_child_pid = pid;
        }

        // Put child in the same process group as first child
        // This allows the shell to control the entire pipeline as one job
        if (setpgid(pid, first_child_pid) != 0)
        {
            perror("myshell: setpgid");
        }

        child_pids.push_back(pid);
    }

    // Register the pipeline as a job with the job manager
    Job job;
    job.command = pipeline.commands.front().program;
    job.background = pipeline.background;
    job.pgid = first_child_pid;
    job.pids = child_pids;
    job.state = JobState::Running;
    JobManager::instance().add(job);

    // Close all pipe file descriptors in parent
    // Parent doesn't need them anymore
    for (int fd : pipe_fds)
    {
        close(fd);
    }

    // If background job, return immediately without waiting
    if (pipeline.background)
    {
        return 0;
    }

    // Give the pipeline control of the terminal
    JobManager::instance().set_foreground_group(first_child_pid);

    // Wait for all children to complete
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

        // Extract exit status from wait status
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

        // Mark child as done and remove from job list
        JobManager::instance().mark_done(pid);
        JobManager::instance().remove(pid);
    }

    // Restore terminal control to the shell
    JobManager::instance().restore_foreground_group();

    return final_status;
#endif
}

} // namespace

/**
 * @brief Main entry point for command execution
 * 
 * This function determines how to execute a pipeline based on its structure:
 * - Empty pipeline: do nothing
 * - Single command: use execute_command()
 * - Multiple commands (pipeline): use execute_pipeline()
 * 
 * Key concepts:
 * - Pipeline can contain 1+ commands
 * - Single commands are simpler (no pipes needed)
 * - Multiple commands require pipe setup and coordination
 * 
 * @param pipeline The parsed pipeline to execute
 * @return Exit status (0 on success, non-zero on failure)
 */
int Executor::execute(const Pipeline& pipeline) const
{
    // Empty pipeline - nothing to do
    if (pipeline.commands.empty())
    {
        return 0;
    }

    // Single command - simpler execution path
    if (pipeline.commands.size() == 1)
    {
        return execute_command(pipeline);
    }

    // Multiple commands - pipeline execution with pipes
    return execute_pipeline(pipeline);
}

/**
 * @brief Execute a single command (built-in or external)
 * 
 * This function handles execution of individual commands, determining
 * whether to use built-in execution or external process creation.
 * 
 * Key concepts:
 * - Built-ins run in shell process (no fork needed)
 * - External commands require fork/exec/wait
 * - Redirections handled differently for built-ins vs external
 * - Background execution creates detached child process
 * 
 * @param pipeline The pipeline containing a single command
 * @return Exit status (0 on success, non-zero on failure)
 */
int Executor::execute_command(const Pipeline& pipeline) const
{
    const Command& command = pipeline.commands[0];

    // BUILT-IN COMMAND EXECUTION
    if (is_builtin(command))
    {
        // Background execution for built-ins
        if (pipeline.background)
        {
#ifdef _WIN32
            // Windows doesn't support fork, skip background for builtins
            std::cerr << "myshell: background builtins not supported on Windows\n";
            return 1;
#else
            // On Linux, fork a child to run the built-in in background
            pid_t pid = fork();
            if (pid < 0)
            {
                perror("myshell: fork");
                return 1;
            }

            if (pid == 0)
            {
                // Child process
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

            // Parent process
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
#endif
        }

        // Foreground built-in without redirection
        if (pipeline.redirections.empty())
        {
            // Simple case: just execute the built-in
            const bool ok = execute_builtin(command);
            std::cout.flush();
            return ok ? 0 : 1;
        }

        // Foreground built-in with redirection
        // Need to save/restore file descriptors since built-in runs in shell process
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

    // EXTERNAL COMMAND EXECUTION
#ifdef _WIN32
    // Windows-specific execution using system() for educational purposes
    // Note: This is not as secure as fork/exec but works for Windows testing
    // In production, you would use CreateProcess() instead
    std::string full_command = command.program;
    for (const auto& arg : command.arguments)
    {
        full_command += " ";
        full_command += arg;
    }

    // Handle redirections for Windows
    if (!pipeline.redirections.empty())
    {
        std::vector<int> saved_fds;
        if (!apply_redirections_to_parent(pipeline, saved_fds))
        {
            return 1;
        }

        int result = system(full_command.c_str());
        restore_parent_redirections(saved_fds);
        return result;
    }

    return system(full_command.c_str());
#else
    // Linux/POSIX implementation using fork/exec/wait
    
    // Fork the process - this creates a child process that's a copy of the parent
    pid_t pid = fork();

    if (pid < 0)
    {
        // Fork failed - report error and return failure
        perror("myshell: fork");
        return 1;
    }

    if (pid == 0)
    {
        // CHILD PROCESS CODE
        // This code runs only in the child process
        
        // Apply I/O redirections (stdin/stdout/stderr)
        if (!apply_redirections_to_child(pipeline))
        {
            _exit(1); // Exit child if redirection fails
        }

        // Reset signal handlers to default for child process
        // This allows Ctrl+C to affect the child, not the shell
        signal(SIGINT, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);

        // Execute the external command
        // This replaces the child process with the new program
        run_child_external(command);
        // If we get here, execvp() failed (run_child_external handles this)
    }

    // PARENT PROCESS CODE
    // This code runs only in the parent (shell) process
    
    // Put the child in its own process group
    // This is important for job control (Ctrl+C affects entire group)
    if (setpgid(pid, pid) != 0)
    {
        perror("myshell: setpgid");
    }

    // Register the child as a job with the job manager
    Job job;
    job.command = command.program;
    job.background = pipeline.background;
    job.pgid = pid;
    job.pids.push_back(pid);
    job.state = JobState::Running;
    JobManager::instance().add(job);

    // Handle foreground vs background execution
    if (!pipeline.background)
    {
        // Foreground job - give it terminal control
        JobManager::instance().set_foreground_group(pid);
    }

    int status = 0;
    if (!pipeline.background)
    {
        // Wait for the child process to complete
        // This blocks the shell until the child finishes
        if (waitpid(pid, &status, 0) < 0)
        {
            perror("myshell: waitpid");
            return 1;
        }

        // Child finished - restore terminal control to shell
        JobManager::instance().restore_foreground_group();
        
        // Mark job as done and remove from job list
        JobManager::instance().mark_done(pid);
        JobManager::instance().remove(pid);

        // Extract and return the child's exit status
        if (WIFEXITED(status))
        {
            return WEXITSTATUS(status);
        }

        // Child terminated abnormally
        return 1;
    }

    // Background job - don't wait, return immediately
    return 0;
#endif
}

} // namespace myshell