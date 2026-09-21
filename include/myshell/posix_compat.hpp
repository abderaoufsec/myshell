#pragma once

/**
 * @file posix_compat.hpp
 * @brief Cross-platform compatibility layer for POSIX APIs
 * 
 * This header provides Windows/MSYS2 equivalents for POSIX functions that
 * are not natively available on Windows. It allows the shell to compile and
 * run on both Linux and Windows platforms.
 * 
 * Key concepts:
 * - POSIX: Portable Operating System Interface (Linux/Unix standard)
 * - Cross-platform: Code that works on multiple operating systems
 * - Conditional compilation: Different code for different platforms
 * - API compatibility: Providing the same interface on different platforms
 * 
 * Platform differences:
 * - Linux: Native POSIX support (fork, exec, pipes, signals, etc.)
 * - Windows: Different process model (CreateProcess vs fork)
 * - This file bridges the gap for educational purposes
 * 
 * Windows implementation notes:
 * - True fork() is not possible on Windows
 * - Uses educational stubs or Windows equivalents
 * - Some features are simplified for learning purposes
 * - Linux code is preserved via #ifdef _WIN32
 * 
 * Usage:
 * Just include this header instead of individual POSIX headers.
 * The appropriate implementation will be selected based on platform.
 */

#ifdef _WIN32
    // WINDOWS IMPLEMENTATION
    // Windows doesn't have native POSIX support, so we provide equivalents
    
    #include <windows.h>
    #include <process.h>
    #include <io.h>
    #include <fcntl.h>
    #include <sys/stat.h>
    
    /**
     * @brief POSIX signal definitions for Windows
     * 
     * Windows has different signal numbers than POSIX. These definitions
     * map POSIX signal numbers to values the Windows implementation can use.
     * 
     * Signals:
     * - SIGINT: Interrupt signal (Ctrl+C)
     * - SIGQUIT: Quit signal (Ctrl+\)
     * - SIGTERM: Termination signal
     * - SIGCHLD: Child process state change
     * - SIGTSTP: Stop signal (Ctrl+Z)
     * 
     * Signal handlers:
     * - SIG_DFL: Default signal handling
     * - SIG_IGN: Ignore signal
     * - SIG_ERR: Error return value
     */
    #define SIGINT 2
    #define SIGQUIT 3
    #define SIGTERM 15
    #define SIGCHLD 17
    #define SIGTSTP 18
    #define SIG_DFL ((void (*)(int))0)
    #define SIG_IGN ((void (*)(int))1)
    #define SIG_ERR ((void (*)(int))-1)
    
    /**
     * @brief POSIX file descriptor definitions
     * 
     * Standard file descriptors used by POSIX programs:
     * - STDIN_FILENO (0): Standard input (keyboard by default)
     * - STDOUT_FILENO (1): Standard output (screen by default)
     * - STDERR_FILENO (2): Standard error (screen by default)
     */
    #define STDIN_FILENO 0
    #define STDOUT_FILENO 1
    #define STDERR_FILENO 2
    
    /**
     * @brief POSIX open flags mapped to Windows equivalents
     * 
     * These flags control how files are opened:
     * - O_RDONLY: Open for reading only
     * - O_WRONLY: Open for writing only
     * - O_RDWR: Open for both reading and writing
     * - O_CREAT: Create file if it doesn't exist
     * - O_TRUNC: Truncate file to zero length (overwrite)
     * - O_APPEND: Append to end of file (preserve existing)
     */
    #ifndef O_RDONLY
        #define O_RDONLY _O_RDONLY
    #endif
    #ifndef O_WRONLY
        #define O_WRONLY _O_WRONLY
    #endif
    #ifndef O_RDWR
        #define O_RDWR _O_RDWR
    #endif
    #ifndef O_CREAT
        #define O_CREAT _O_CREAT
    #endif
    #ifndef O_TRUNC
        #define O_TRUNC _O_TRUNC
    #endif
    #ifndef O_APPEND
        #define O_APPEND _O_APPEND
    #endif
    
    /**
     * @brief POSIX mode_t definition for Windows
     * 
     * On POSIX systems, mode_t is used for file permissions (e.g., 0644).
     * Windows uses a different system, so we map it to unsigned short.
     */
    using mode_t = unsigned short;
    
    /**
     * @brief POSIX function prototypes for Windows
     * 
     * These functions provide Windows implementations of POSIX APIs.
     * They are implemented in posix_compat.cpp.
     * 
     * Key functions:
     * - fork(): Create child process (educational stub on Windows)
     * - waitpid(): Wait for child process to change state
     * - pipe(): Create inter-process communication channel
     * - setpgid(): Set process group ID
     * - getpgrp(): Get process group ID
     * - tcsetpgrp(): Set foreground process group for terminal
     * - isatty(): Check if file descriptor is a terminal
     * - setenv(): Set environment variable
     * - unsetenv(): Remove environment variable
     * - signal(): Set signal handler
     * - kill(): Send signal to process
     */
    extern "C" {
        pid_t fork();
        pid_t waitpid(pid_t pid, int* status, int options);
        int pipe(int pipefd[2]);
        int setpgid(pid_t pid, pid_t pgid);
        pid_t getpgrp();
        int tcsetpgrp(int fd, pid_t pgid);
        int isatty(int fd);
        int setenv(const char* name, const char* value, int overwrite);
        int unsetenv(const char* name);
        void (*signal(int sig, void (*func)(int)))(int);
        int kill(pid_t pid, int sig);
    }
    
    /**
     * @brief POSIX wait status macros for Windows
     * 
     * These macros extract information from the status returned by waitpid():
     * - WIFEXITED: Check if process exited normally
     * - WEXITSTATUS: Extract exit code from normally exited process
     * - WIFSIGNALED: Check if process was terminated by signal
     * - WTERMSIG: Extract signal number that terminated process
     */
    #define WIFEXITED(status) (((status) & 0x7f) == 0)
    #define WEXITSTATUS(status) (((status) >> 8) & 0xff)
    #define WIFSIGNALED(status) (((signed char)(((status) & 0x7f) + 1) >> 1) > 0)
    #define WTERMSIG(status) ((status) & 0x7f)
    
    /**
     * @brief POSIX file descriptor functions mapped to Windows
     * 
     * Windows uses underscore-prefixed versions of these functions:
     * - dup(): Duplicate file descriptor
     * - dup2(): Duplicate file descriptor to specific descriptor
     * - close(): Close file descriptor
     * - read(): Read from file descriptor
     * - write(): Write to file descriptor
     * - open(): Open file and return file descriptor
     * - unlink(): Delete file
     */
    #define dup _dup
    #define dup2 _dup2
    #define close _close
    #define read _read
    #define write _write
    #define open _open
    #define unlink _unlink
    
#else
    // LINUX/POSIX IMPLEMENTATION
    // On Linux/POSIX systems, use the native headers directly
    // This provides the real, full-featured POSIX APIs
    
    #include <sys/wait.h>   // Process waiting and status macros
    #include <sys/types.h>  // System data types (pid_t, etc.)
    #include <unistd.h>     // POSIX API (fork, exec, pipe, etc.)
    #include <signal.h>     // Signal handling
    #include <fcntl.h>      // File control operations (open, dup2, etc.)
    #include <termios.h>    // Terminal I/O (tcsetpgrp, etc.)
    #include <stdlib.h>     // Environment variables (setenv, unsetenv)
#endif