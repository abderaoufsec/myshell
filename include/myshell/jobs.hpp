#pragma once

#include "myshell/types.hpp"

#include <string>
#include <sys/types.h>
#include <vector>

namespace myshell
{

enum class JobState
{
    Running,
    Stopped,
    Done
};

struct Job
{
    int id = -1;
    std::string command;
    std::vector<pid_t> pids;
    pid_t pgid = -1;
    JobState state = JobState::Running;
    bool background = false;
};

class JobManager
{
public:
    static JobManager& instance();

    void add(const Job& job);
    void remove(pid_t pid);
    void update_state(pid_t pid, JobState state);
    void update_state_by_pgid(pid_t pgid, JobState state);
    void mark_done(pid_t pid);
    void mark_running(pid_t pid);
    void mark_stopped(pid_t pid);
    Job* find_by_pid(pid_t pid);
    const Job* find_by_pid(pid_t pid) const;
    Job* find_by_pgid(pid_t pgid);
    const Job* find_by_pgid(pid_t pgid) const;
    void set_foreground_group(pid_t pgid);
    void restore_foreground_group();
    const std::vector<Job>& jobs() const;

private:
    std::vector<Job> jobs_;
    int next_job_id_ = 1;
    pid_t foreground_pgid_ = -1;
};

} // namespace myshell
