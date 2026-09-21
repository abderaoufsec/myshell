#pragma once

#include "myshell/types.hpp"

#include <string>
#include <sys/types.h>
#include <vector>

namespace myshell
{

/**
 * @brief Enumeration of possible job states
 * 
 * Jobs (background processes and pipelines) can be in different states
 * during their lifecycle. This enum tracks the current state.
 * 
 * Key concepts:
 * - Running: Process is currently executing
 * - Stopped: Process is paused (e.g., Ctrl+Z signal)
 * - Done: Process has completed (normally or abnormally)
 * 
 * Lifecycle: Running -> Stopped -> Done
 *                 -> Done (direct)
 */
enum class JobState
{
    Running, // Process is currently executing
    Stopped, // Process is paused (e.g., SIGTSTP signal)
    Done     // Process has completed
};

/**
 * @brief Represents a background job or pipeline
 * 
 * This structure holds information about a job (group of related processes)
 * that the shell is tracking for job control purposes.
 * 
 * Key concepts:
 * - Job ID: Sequential number assigned by shell (e.g., [1], [2])
 * - Command: String representation of the job (for display)
 * - PIDs: All process IDs in the job (pipelines have multiple)
 * - PGID: Process Group ID (all processes in job share same PGID)
 * - State: Current execution state (Running, Stopped, Done)
 * - Background: Whether job runs in background (&)
 * 
 * Example: "sleep 10 &" becomes a job with:
 * - id = 1
 * - command = "sleep 10"
 * - pids = [12345]
 * - pgid = 12345
 * - state = Running
 * - background = true
 * 
 * Example: "ls | grep txt" becomes a job with:
 * - id = 2
 * - command = "ls"
 * - pids = [12346, 12347] (two processes)
 * - pgid = 12346 (both in same process group)
 * - state = Running
 * - background = false
 */
struct Job
{
    int id = -1;                       // Job ID assigned by shell (sequential)
    std::string command;              // Command string for display
    std::vector<pid_t> pids;          // All process IDs in this job
    pid_t pgid = -1;                  // Process Group ID (shared by all processes)
    JobState state = JobState::Running; // Current state of the job
    bool background = false;           // Whether job runs in background
};

/**
 * @brief Manages background jobs and process groups
 * 
 * This class provides job control functionality, allowing the shell to:
 * - Track background processes
 * - Manage process groups for terminal control
 * - Handle foreground/background job switching
 * - Implement signals like Ctrl+C and Ctrl+Z
 * 
 * Key concepts:
 * - Singleton pattern: Only one JobManager exists per shell
 * - Job control: Managing which process gets terminal input
 * - Process groups: Groups of processes managed together
 * - Terminal control: tcsetpgrp for foreground/background switching
 * 
 * Example flow for "sleep 10 &":
 * 1. Shell forks child process
 * 2. JobManager.add() creates job with child's PID
 * 3. Shell returns to prompt immediately (background)
 * 4. When child finishes, JobManager.remove() cleans up
 * 
 * Example flow for "sleep 10":
 * 1. Shell forks child process
 * 2. JobManager.add() creates job
 * 3. JobManager.set_foreground_group() gives child terminal control
 * 4. Shell waits for child to complete
 * 5. JobManager.restore_foreground_group() returns control to shell
 * 6. JobManager.remove() cleans up
 */
class JobManager
{
public:
    /**
     * @brief Get the singleton instance of JobManager
     * 
     * @return Reference to the singleton JobManager instance
     */
    static JobManager& instance();

    /**
     * @brief Add a new job to the job manager
     * 
     * @param job The job to add (may be normalized)
     */
    void add(const Job& job);

    /**
     * @brief Remove a job by process ID
     * 
     * @param pid The process ID of the job to remove
     */
    void remove(pid_t pid);

    /**
     * @brief Update job state by process ID
     * 
     * @param pid The process ID to search for
     * @param state The new state to set
     */
    void update_state(pid_t pid, JobState state);

    /**
     * @brief Update job state by process group ID
     * 
     * @param pgid The process group ID to search for
     * @param state The new state to set
     */
    void update_state_by_pgid(pid_t pgid, JobState state);

    /**
     * @brief Mark a job as Done by process ID
     * 
     * @param pid The process ID of the completed job
     */
    void mark_done(pid_t pid);

    /**
     * @brief Mark a job as Running by process ID
     * 
     * @param pid The process ID of the running job
     */
    void mark_running(pid_t pid);

    /**
     * @brief Mark a job as Stopped by process ID
     * 
     * @param pid The process ID of the stopped job
     */
    void mark_stopped(pid_t pid);

    /**
     * @brief Find a job by process ID (non-const)
     * 
     * @param pid The process ID to search for
     * @return Pointer to the job, or nullptr if not found
     */
    Job* find_by_pid(pid_t pid);

    /**
     * @brief Find a job by process ID (const)
     * 
     * @param pid The process ID to search for
     * @return Const pointer to the job, or nullptr if not found
     */
    const Job* find_by_pid(pid_t pid) const;

    /**
     * @brief Find a job by process group ID (non-const)
     * 
     * @param pgid The process group ID to search for
     * @return Pointer to the job, or nullptr if not found
     */
    Job* find_by_pgid(pid_t pgid);

    /**
     * @brief Find a job by process group ID (const)
     * 
     * @param pgid The process group ID to search for
     * @return Const pointer to the job, or nullptr if not found
     */
    const Job* find_by_pgid(pid_t pgid) const;

    /**
     * @brief Set the foreground process group for terminal control
     * 
     * @param pgid The process group ID to make foreground
     */
    void set_foreground_group(pid_t pgid);

    /**
     * @brief Restore terminal control to the shell
     */
    void restore_foreground_group();

    /**
     * @brief Get all jobs
     * 
     * @return Const reference to the vector of all jobs
     */
    const std::vector<Job>& jobs() const;

private:
    std::vector<Job> jobs_;        // All tracked jobs
    int next_job_id_ = 1;          // Next job ID to assign
    pid_t foreground_pgid_ = -1;    // Current foreground process group
};

} // namespace myshell
