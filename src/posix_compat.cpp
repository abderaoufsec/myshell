#include "myshell/posix_compat.hpp"

#ifdef _WIN32

/**
 * @file posix_compat.cpp
 * @brief Windows implementations of POSIX functions
 * 
 * This file provides Windows implementations of POSIX APIs that are not
 * natively available on Windows. These implementations are designed for
 * educational purposes and provide basic compatibility for the shell project.
 * 
 * IMPORTANT: These are educational implementations, not production-quality.
 * For production Windows code, you would use proper Windows APIs throughout.
 */

#include <cstring>
#include <cerrno>
#include <vector>
#include <memory>
#include <algorithm>

// Global state for signal handling
// Windows signal handling is different from POSIX, so we store handlers here
static void (*g_signal_handlers[32])(int) = {nullptr};

/**
 * @brief Windows implementation of fork()
 * 
 * fork() creates a copy of the current process. This is not possible on
 * Windows in the same way as POSIX. Windows uses CreateProcess() instead,
 * which has a different model (parent creates child, not a copy).
 * 
 * For educational purposes, this returns an error since the shell is
 * designed for Linux/POSIX systems. The Windows execution path uses
 * system() instead of fork/exec.
 * 
 * @return -1 (error) - fork is not supported on Windows
 */
pid_t fork()
{
    // fork() is not supported on Windows
    // For educational purposes, we return an error
    // The project is designed for Linux/POSIX systems
    errno = ENOSYS; // Function not implemented
    return -1;
}

/**
 * @brief Windows implementation of waitpid()
 * 
 * waitpid() waits for a child process to change state. On Windows, we use
 * WaitForSingleObject() to wait for process completion.
 * 
 * Key concepts:
 * - OpenProcess(): Gets a handle to the process
 * - WaitForSingleObject(): Waits for process to finish
 * - GetExitCodeProcess(): Gets the process exit code
 * - Status conversion: Windows exit code to POSIX status format
 * 
 * @param pid Process ID to wait for
 * @param status Pointer to store exit status (can be nullptr)
 * @param options Wait options (must be 0 for this implementation)
 * @return Process ID on success, -1 on error
 */
pid_t waitpid(pid_t pid, int* status, int options)
{
    // This implementation doesn't support options like WNOHANG
    if (options != 0)
    {
        errno = EINVAL;
        return -1;
    }
    
    // Open a handle to the process with query and synchronize permissions
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
    if (hProcess == NULL)
    {
        errno = ECHILD; // No child process
        return -1;
    }
    
    // Wait for the process to finish (infinite wait)
    DWORD waitResult = WaitForSingleObject(hProcess, INFINITE);
    if (waitResult != WAIT_OBJECT_0)
    {
        CloseHandle(hProcess);
        errno = EINTR; // Interrupted system call
        return -1;
    }
    
    // Get the process exit code
    DWORD exitCode;
    if (!GetExitCodeProcess(hProcess, &exitCode))
    {
        CloseHandle(hProcess);
        errno = ECHILD;
        return -1;
    }
    
    // Close the process handle
    CloseHandle(hProcess);
    
    // Convert Windows exit code to POSIX-style status
    // POSIX status has exit code in bits 8-15
    if (status != nullptr)
    {
        *status = exitCode << 8;
    }
    
    return pid;
}

/**
 * @brief Windows implementation of pipe()
 * 
 * pipe() creates an anonymous pipe for inter-process communication.
 * On Windows, we use CreatePipe() and convert handles to C file descriptors.
 * 
 * Key concepts:
 * - CreatePipe(): Creates Windows pipe with read/write handles
 * - _open_osfhandle(): Converts Windows handle to C file descriptor
 * - pipefd[0]: Read end of pipe
 * - pipefd[1]: Write end of pipe
 * 
 * @param pipefd Array to store read and write file descriptors
 * @return 0 on success, -1 on error
 */
int pipe(int pipefd[2])
{
    HANDLE readHandle, writeHandle;
    
    // Create an anonymous pipe
    if (!CreatePipe(&readHandle, &writeHandle, NULL, 0))
    {
        errno = GetLastError();
        return -1;
    }
    
    // Convert Windows handles to C file descriptors
    // _open_osfhandle() attaches a C runtime file descriptor to a Windows handle
    int readFd = _open_osfhandle(reinterpret_cast<intptr_t>(readHandle), _O_RDONLY);
    int writeFd = _open_osfhandle(reinterpret_cast<intptr_t>(writeHandle), _O_WRONLY);
    
    // Check for conversion errors
    if (readFd == -1 || writeFd == -1)
    {
        if (readFd != -1) _close(readFd);
        if (writeFd != -1) _close(writeFd);
        CloseHandle(readHandle);
        CloseHandle(writeHandle);
        errno = EMFILE; // Too many open files
        return -1;
    }
    
    // Store the file descriptors in the output array
    pipefd[0] = readFd;   // Read end
    pipefd[1] = writeFd;  // Write end
    
    return 0;
}

/**
 * @brief Windows implementation of setpgid()
 * 
 * setpgid() sets the process group ID for a process. Windows has a different
 * concept of process groups (Job Objects), so this is a stub for educational
 * purposes.
 * 
 * @param pid Process ID (ignored on Windows)
 * @param pgid Process group ID (ignored on Windows)
 * @return 0 (success - ignored on Windows)
 */
int setpgid(pid_t pid, pid_t pgid)
{
    // Process groups are not fully supported on Windows in the POSIX sense
    // Windows uses Job Objects for similar functionality
    // This is a stub for educational purposes
    (void)pid;
    (void)pgid;
    return 0; // Success (operation ignored on Windows)
}

/**
 * @brief Windows implementation of getpgrp()
 * 
 * getpgrp() returns the process group ID of the calling process.
 * On Windows, we return the current process ID as a simple approximation.
 * 
 * @return Current process ID (as approximation of process group ID)
 */
pid_t getpgrp()
{
    // Return current process ID as process group ID
    // This is a simplification for educational purposes
    return GetCurrentProcessId();
}

/**
 * @brief Windows implementation of tcsetpgrp()
 * 
 * tcsetpgrp() sets the foreground process group for terminal control.
 * Windows terminal control works differently, so this is a stub.
 * 
 * @param fd File descriptor (ignored on Windows)
 * @param pgid Process group ID (ignored on Windows)
 * @return 0 (success - ignored on Windows)
 */
int tcsetpgrp(int fd, pid_t pgid)
{
    // Terminal control is not applicable on Windows in the same way
    // Windows console handling is different from POSIX terminals
    // This is a stub for educational purposes
    (void)fd;
    (void)pgid;
    return 0; // Success (operation ignored on Windows)
}

/**
 * @brief Windows implementation of isatty()
 * 
 * isatty() checks if a file descriptor refers to a terminal.
 * On Windows, we use the _isatty() function from the MSVC runtime.
 * 
 * @param fd File descriptor to check
 * @return 1 if terminal, 0 if not
 */
int isatty(int fd)
{
    return _isatty(fd);
}

/**
 * @brief Windows implementation of setenv()
 * 
 * setenv() sets or changes an environment variable. On Windows, we use
 * _putenv_s() from the MSVC runtime.
 * 
 * @param name Environment variable name
 * @param value Environment variable value
 * @param overwrite Whether to overwrite if variable exists (1=yes, 0=no)
 * @return 0 on success, -1 on error
 */
int setenv(const char* name, const char* value, int overwrite)
{
    // Validate input
    if (name == nullptr || name[0] == '\0')
    {
        errno = EINVAL;
        return -1;
    }
    
    // Check if variable exists and overwrite is disabled
    if (!overwrite && getenv(name) != nullptr)
    {
        return 0; // Already exists and overwrite is disabled
    }
    
    // Set the environment variable using Windows API
    if (_putenv_s(name, value) != 0)
    {
        errno = ENOMEM;
        return -1;
    }
    
    return 0;
}

/**
 * @brief Windows implementation of unsetenv()
 * 
 * unsetenv() removes an environment variable. On Windows, we use
 * _putenv_s() with an empty string to remove the variable.
 * 
 * @param name Environment variable name to remove
 * @return 0 on success, -1 on error
 */
int unsetenv(const char* name)
{
    // Validate input
    if (name == nullptr || name[0] == '\0')
    {
        errno = EINVAL;
        return -1;
    }
    
    // Remove the environment variable by setting it to empty string
    if (_putenv_s(name, "") != 0)
    {
        errno = ENOMEM;
        return -1;
    }
    
    return 0;
}

/**
 * @brief Windows implementation of signal()
 * 
 * signal() sets a signal handler for a specific signal. Windows signal
 * handling is more limited than POSIX, so this is a simplified implementation
 * that just stores the handler for educational purposes.
 * 
 * @param sig Signal number (SIGINT, SIGQUIT, etc.)
 * @param func Signal handler function
 * @return Previous signal handler, or SIG_ERR on error
 */
void (*signal(int sig, void (*func)(int)))(int)
{
    // Validate signal number
    if (sig < 0 || sig >= 32)
    {
        errno = EINVAL;
        return (void (*)(int))-1; // SIG_ERR
    }
    
    // Store the previous handler
    void (*old_handler)(int) = g_signal_handlers[sig];
    
    // Set the new handler
    g_signal_handlers[sig] = func;
    
    // Note: Windows signal handling is limited compared to POSIX
    // For educational purposes, we just store the handler
    // Real signal handling would require Windows-specific APIs
    // and would be much more complex
    
    return old_handler;
}

/**
 * @brief Windows implementation of kill()
 * 
 * kill() sends a signal to a process. On Windows, we use TerminateProcess()
 * to terminate a process (similar to SIGKILL).
 * 
 * @param pid Process ID to send signal to
 * @param sig Signal number (used as exit code in Windows)
 * @return 0 on success, -1 on error
 */
int kill(pid_t pid, int sig)
{
    // Open the process with terminate permission
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess == NULL)
    {
        errno = ESRCH; // No such process
        return -1;
    }
    
    // Terminate the process
    // We use the signal number as the exit code for educational purposes
    if (!TerminateProcess(hProcess, sig))
    {
        CloseHandle(hProcess);
        errno = EPERM; // Operation not permitted
        return -1;
    }
    
    // Close the process handle
    CloseHandle(hProcess);
    return 0;
}

#endif // _WIN32