#include "myshell/jobs.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <termios.h>

namespace myshell
{
namespace
{

bool should_control_terminal()
{
    const char* interactive = std::getenv("MYSHELL_INTERACTIVE");
    return interactive != nullptr && interactive[0] == '1' && isatty(STDIN_FILENO);
}

} // namespace

JobManager& JobManager::instance()
{
    static JobManager manager;
    return manager;
}

void JobManager::add(const Job& job)
{
    Job normalized = job;
    if (normalized.id < 0)
    {
        normalized.id = next_job_id_++;
    }

    if (normalized.pgid == -1 && !normalized.pids.empty())
    {
        normalized.pgid = normalized.pids.front();
    }

    if (normalized.command.empty())
    {
        normalized.command = "<unknown>";
    }

    auto existing = std::find_if(jobs_.begin(), jobs_.end(),
        [&normalized](const Job& existing_job)
        {
            return existing_job.id == normalized.id ||
                   existing_job.pgid == normalized.pgid ||
                   (existing_job.pids == normalized.pids);
        });

    if (existing != jobs_.end())
    {
        *existing = normalized;
        return;
    }

    jobs_.push_back(normalized);
}

void JobManager::remove(pid_t pid)
{
    auto it = std::remove_if(jobs_.begin(), jobs_.end(),
        [pid](const Job& job)
        {
            return std::find(job.pids.begin(), job.pids.end(), pid) != job.pids.end();
        });

    if (it != jobs_.end())
    {
        jobs_.erase(it, jobs_.end());
    }
}

void JobManager::update_state(pid_t pid, JobState state)
{
    for (Job& job : jobs_)
    {
        if (std::find(job.pids.begin(), job.pids.end(), pid) != job.pids.end())
        {
            job.state = state;
            return;
        }
    }
}

void JobManager::update_state_by_pgid(pid_t pgid, JobState state)
{
    for (Job& job : jobs_)
    {
        if (job.pgid == pgid)
        {
            job.state = state;
            return;
        }
    }
}

void JobManager::mark_done(pid_t pid)
{
    update_state(pid, JobState::Done);
}

void JobManager::mark_running(pid_t pid)
{
    update_state(pid, JobState::Running);
}

void JobManager::mark_stopped(pid_t pid)
{
    update_state(pid, JobState::Stopped);
}

Job* JobManager::find_by_pid(pid_t pid)
{
    auto it = std::find_if(jobs_.begin(), jobs_.end(),
        [pid](const Job& job)
        {
            return std::find(job.pids.begin(), job.pids.end(), pid) != job.pids.end();
        });

    return it != jobs_.end() ? &(*it) : nullptr;
}

const Job* JobManager::find_by_pid(pid_t pid) const
{
    auto it = std::find_if(jobs_.begin(), jobs_.end(),
        [pid](const Job& job)
        {
            return std::find(job.pids.begin(), job.pids.end(), pid) != job.pids.end();
        });

    return it != jobs_.end() ? &(*it) : nullptr;
}

Job* JobManager::find_by_pgid(pid_t pgid)
{
    auto it = std::find_if(jobs_.begin(), jobs_.end(),
        [pgid](const Job& job)
        {
            return job.pgid == pgid;
        });

    return it != jobs_.end() ? &(*it) : nullptr;
}

const Job* JobManager::find_by_pgid(pid_t pgid) const
{
    auto it = std::find_if(jobs_.begin(), jobs_.end(),
        [pgid](const Job& job)
        {
            return job.pgid == pgid;
        });

    return it != jobs_.end() ? &(*it) : nullptr;
}

void JobManager::set_foreground_group(pid_t pgid)
{
    foreground_pgid_ = pgid;

    if (!should_control_terminal())
    {
        return;
    }

    if (tcsetpgrp(STDIN_FILENO, pgid) == -1)
    {
        std::cerr << "myshell: tcsetpgrp: " << strerror(errno) << '\n';
    }
}

void JobManager::restore_foreground_group()
{
    if (!should_control_terminal())
    {
        return;
    }

    const pid_t shell_pgid = getpgrp();
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) == -1)
    {
        std::cerr << "myshell: tcsetpgrp: " << strerror(errno) << '\n';
    }
    foreground_pgid_ = -1;
}

const std::vector<Job>& JobManager::jobs() const
{
    return jobs_;
}

} // namespace myshell
