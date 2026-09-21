#include "myshell/jobs.hpp"
#include "myshell/posix_compat.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <iostream>

namespace myshell
{
namespace
{

/**
 * @brief Check if the shell should control the terminal
 * 
 * Terminal control is only needed in interactive mode when stdin is a tty.
 * This prevents terminal control operations in non-interactive contexts
 * (like when the shell is reading from a file or pipe).
 * 
 * Key concepts:
 * - TTY: Teletype, a terminal device
 * - isatty(): Checks if a file descriptor refers to a terminal
 * - Interactive mode: User is typing commands directly
 * - Non-interactive: Commands coming from script/pipe
 * 
 * @return true if shell should control terminal, false otherwise
 */
bool should_control_terminal()
{
    // Check for interactive mode flag
    const char* interactive = std::getenv("MYSHELL_INTERACTIVE");
    
    // Should control terminal if:
    // 1. Interactive flag is set to "1"
    // 2. stdin is connected to a terminal (not a file/pipe)
    return interactive != nullptr && interactive[0] == '1' && isatty(STDIN_FILENO);
}

} // namespace

/**
 * @brief Get the singleton instance of JobManager
 * 
 * This implements the Singleton pattern - there's only one JobManager
 * for the entire shell process. This ensures all job tracking is centralized.
 * 
 * Key concepts:
 * - Singleton: Only one instance exists globally
 * - Static local variable: Initialized on first use, thread-safe in C++11+
 * - Reference return: Allows modification of the singleton
 * 
 * @return Reference to the singleton JobManager instance
 */
JobManager& JobManager::instance()
{
    static JobManager manager; // Created once on first call
    return manager;
}

/**
 * @brief Add a job to the job manager
 * 
 * This function registers a new job (background process or pipeline) with
 * the job manager. It normalizes the job data and checks for duplicates.
 * 
 * Key concepts:
 * - Job: A group of related processes (e.g., a pipeline)
 * - Job ID: Sequential number assigned to each job
 * - Process Group ID (PGID): All processes in a job share the same PGID
 * - Normalization: Fill in missing fields with defaults
 * 
 * @param job The job to add (may be incomplete/normalized)
 */
void JobManager::add(const Job& job)
{
    // Create a normalized copy of the job
    Job normalized = job;
    
    // Assign a job ID if not provided
    if (normalized.id < 0)
    {
        normalized.id = next_job_id_++;
    }

    // Set process group ID if not provided
    // Use the first process's PID as the PGID for the entire job
    if (normalized.pgid == -1 && !normalized.pids.empty())
    {
        normalized.pgid = normalized.pids.front();
    }

    // Provide default command name if not provided
    if (normalized.command.empty())
    {
        normalized.command = "<unknown>";
    }

    // Check if this job already exists (by ID, PGID, or PIDs)
    // This prevents duplicate entries
    auto existing = std::find_if(jobs_.begin(), jobs_.end(),
        [&normalized](const Job& existing_job)
        {
            return existing_job.id == normalized.id ||
                   existing_job.pgid == normalized.pgid ||
                   (existing_job.pids == normalized.pids);
        });

    // If job exists, update it; otherwise add as new job
    if (existing != jobs_.end())
    {
        *existing = normalized; // Update existing job
        return;
    }

    jobs_.push_back(normalized); // Add new job
}

/**
 * @brief Remove a job from the job manager
 * 
 * This function removes a job that contains the specified process ID.
 * It's typically called when a process has completed and is being cleaned up.
 * 
 * Key concepts:
 * - Jobs can contain multiple processes (pipelines)
 * - Removing by PID cleans up the entire job if any of its processes match
 * - std::remove_if: Moves matching elements to end, returns new end iterator
 * - erase: Actually removes the elements from the container
 * 
 * @param pid The process ID to search for
 */
void JobManager::remove(pid_t pid)
{
    // Find and remove jobs that contain the specified PID
    auto it = std::remove_if(jobs_.begin(), jobs_.end(),
        [pid](const Job& job)
        {
            // Check if this job contains the PID
            return std::find(job.pids.begin(), job.pids.end(), pid) != job.pids.end();
        });

    // Actually remove the jobs from the vector
    if (it != jobs_.end())
    {
        jobs_.erase(it, jobs_.end());
    }
}

/**
 * @brief Update the state of a job by process ID
 * 
 * This function finds the job containing the specified process ID and
 * updates its state (Running, Stopped, Done).
 * 
 * Key concepts:
 * - Job states track the lifecycle of background processes
 * - Running: Process is currently executing
 * - Stopped: Process is paused (e.g., Ctrl+Z)
 * - Done: Process has completed
 * 
 * @param pid The process ID to search for
 * @param state The new state to set
 */
void JobManager::update_state(pid_t pid, JobState state)
{
    // Search for a job containing this PID
    for (Job& job : jobs_)
    {
        if (std::find(job.pids.begin(), job.pids.end(), pid) != job.pids.end())
        {
            job.state = state; // Update the job's state
            return;
        }
    }
}

/**
 * @brief Update the state of a job by process group ID
 * 
 * This function finds the job with the specified process group ID and
 * updates its state. This is useful for updating all processes in a pipeline.
 * 
 * Key concepts:
 * - Process Group ID (PGID): All processes in a pipeline share the same PGID
 * - Updating by PGID affects the entire job/pipeline at once
 * - More efficient than updating by individual PID
 * 
 * @param pgid The process group ID to search for
 * @param state The new state to set
 */
void JobManager::update_state_by_pgid(pid_t pgid, JobState state)
{
    // Search for a job with this PGID
    for (Job& job : jobs_)
    {
        if (job.pgid == pgid)
        {
            job.state = state; // Update the job's state
            return;
        }
    }
}

/**
 * @brief Mark a job as Done by process ID
 * 
 * Convenience function to mark a job as completed.
 * 
 * @param pid The process ID of the completed job
 */
void JobManager::mark_done(pid_t pid)
{
    update_state(pid, JobState::Done);
}

/**
 * @brief Mark a job as Running by process ID
 * 
 * Convenience function to mark a job as currently executing.
 * 
 * @param pid The process ID of the running job
 */
void JobManager::mark_running(pid_t pid)
{
    update_state(pid, JobState::Running);
}

/**
 * @brief Mark a job as Stopped by process ID
 * 
 * Convenience function to mark a job as paused.
 * 
 * @param pid The process ID of the stopped job
 */
void JobManager::mark_stopped(pid_t pid)
{
    update_state(pid, JobState::Stopped);
}

/**
 * @brief Find a job by process ID (non-const version)
 * 
 * This function searches for a job containing the specified process ID
 * and returns a pointer to it for modification.
 * 
 * Key concepts:
 * - Returns nullptr if job not found
 * - Non-const version allows modification of the job
 * - Useful for updating job state or other properties
 * 
 * @param pid The process ID to search for
 * @return Pointer to the job, or nullptr if not found
 */
Job* JobManager::find_by_pid(pid_t pid)
{
    auto it = std::find_if(jobs_.begin(), jobs_.end(),
        [pid](const Job& job)
        {
            return std::find(job.pids.begin(), job.pids.end(), pid) != job.pids.end();
        });

    return it != jobs_.end() ? &(*it) : nullptr;
}

/**
 * @brief Find a job by process ID (const version)
 * 
 * This function searches for a job containing the specified process ID
 * and returns a const pointer for read-only access.
 * 
 * @param pid The process ID to search for
 * @return Const pointer to the job, or nullptr if not found
 */
const Job* JobManager::find_by_pid(pid_t pid) const
{
    auto it = std::find_if(jobs_.begin(), jobs_.end(),
        [pid](const Job& job)
        {
            return std::find(job.pids.begin(), job.pids.end(), pid) != job.pids.end();
        });

    return it != jobs_.end() ? &(*it) : nullptr;
}

/**
 * @brief Find a job by process group ID (non-const version)
 * 
 * This function searches for a job with the specified process group ID
 * and returns a pointer to it for modification.
 * 
 * Key concepts:
 * - Searching by PGID is more efficient for pipelines
 * - All processes in a pipeline share the same PGID
 * - Returns nullptr if job not found
 * 
 * @param pgid The process group ID to search for
 * @return Pointer to the job, or nullptr if not found
 */
Job* JobManager::find_by_pgid(pid_t pgid)
{
    auto it = std::find_if(jobs_.begin(), jobs_.end(),
        [pgid](const Job& job)
        {
            return job.pgid == pgid;
        });

    return it != jobs_.end() ? &(*it) : nullptr;
}

/**
 * @brief Find a job by process group ID (const version)
 * 
 * This function searches for a job with the specified process group ID
 * and returns a const pointer for read-only access.
 * 
 * @param pgid The process group ID to search for
 * @return Const pointer to the job, or nullptr if not found
 */
const Job* JobManager::find_by_pgid(pid_t pgid) const
{
    auto it = std::find_if(jobs_.begin(), jobs_.end(),
        [pgid](const Job& job)
        {
            return job.pgid == pgid;
        });

    return it != jobs_.end() ? &(*it) : nullptr;
}

/**
 * @brief Set the foreground process group for terminal control
 * 
 * This function gives a specific process group control of the terminal.
 * This is essential for job control - only the foreground process group
 * should receive keyboard input (like Ctrl+C).
 * 
 * Key concepts:
 * - Foreground process group: Receives terminal input
 * - Background process groups: Do not receive terminal input
 * - tcsetpgrp(): POSIX function to set foreground process group
 * - Terminal control: Determines which process gets keyboard input
 * 
 * Example: When you run "sleep 10", the sleep process becomes the
 * foreground group and receives Ctrl+C. When it finishes, control
 * returns to the shell.
 * 
 * @param pgid The process group ID to make foreground
 */
void JobManager::set_foreground_group(pid_t pgid)
{
    // Remember which process group is currently foreground
    foreground_pgid_ = pgid;

    // Only control terminal in interactive mode
    if (!should_control_terminal())
    {
        return;
    }

    // Use tcsetpgrp to give the process group terminal control
    // This makes the specified PGID the foreground process group
    if (tcsetpgrp(STDIN_FILENO, pgid) == -1)
    {
        std::cerr << "myshell: tcsetpgrp: " << strerror(errno) << '\n';
    }
}

/**
 * @brief Restore terminal control to the shell
 * 
 * This function returns terminal control to the shell's process group
 * after a foreground job has completed. This ensures the shell receives
 * keyboard input for the next command.
 * 
 * Key concepts:
 * - Must be called after foreground job completes
 * - Restores shell's ability to receive keyboard input
 * - getpgrp(): Gets the process group ID of the calling process
 * - Prevents shell from losing terminal control
 * 
 * Example: After "sleep 10" completes, this function ensures the shell
 * (not the defunct sleep process) receives your next command.
 */
void JobManager::restore_foreground_group()
{
    // Only control terminal in interactive mode
    if (!should_control_terminal())
    {
        return;
    }

    // Get the shell's process group ID
    const pid_t shell_pgid = getpgrp();
    
    // Return terminal control to the shell's process group
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) == -1)
    {
        std::cerr << "myshell: tcsetpgrp: " << strerror(errno) << '\n';
    }
    
    // Clear the foreground PGID (no foreground job)
    foreground_pgid_ = -1;
}

/**
 * @brief Get the list of all jobs
 * 
 * This function returns a const reference to the internal job list,
 * allowing read-only access to all tracked jobs.
 * 
 * Key concepts:
 * - Const reference: Prevents modification of internal state
 * - Useful for displaying job status (e.g., "jobs" command)
 * - Returns all jobs regardless of state
 * 
 * @return Const reference to the vector of all jobs
 */
const std::vector<Job>& JobManager::jobs() const
{
    return jobs_;
}

} // namespace myshell
