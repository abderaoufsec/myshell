# myshell — Professional Project Documentation

> A Unix-like shell implemented in modern C++20 for Linux.
>
> **Status:** Active development. Current milestone: I/O redirection.
>
> **Purpose:** Learn Unix/POSIX systems programming by implementing a shell from first principles while maintaining professional C++ project structure, testing, debugging, documentation, and Git practices.

---

## Table of Contents

- [1. Overview](#1-overview)
- [2. Goals](#2-goals)
- [3. Technology Stack](#3-technology-stack)
- [4. Environment](#4-environment)
- [5. Architecture](#5-architecture)
- [6. Repository Structure](#6-repository-structure)
- [7. Core Data Model](#7-core-data-model)
- [8. Shell REPL](#8-shell-repl)
- [9. Parser](#9-parser)
- [10. Built-ins](#10-built-ins)
- [11. Processes](#11-processes)
- [12. fork()](#12-fork)
- [13. execvp()](#13-execvp)
- [14. waitpid()](#14-waitpid)
- [15. PATH](#15-path)
- [16. File Descriptors](#16-file-descriptors)
- [17. Output Redirection](#17-output-redirection)
- [18. Append Redirection](#18-append-redirection)
- [19. Input Redirection](#19-input-redirection)
- [20. Built-in Redirection](#20-built-in-redirection)
- [21. Pipes](#21-pipes)
- [22. Environment Variables](#22-environment-variables)
- [23. Quoting and Escaping](#23-quoting-and-escaping)
- [24. Background Processes](#24-background-processes)
- [25. Signals](#25-signals)
- [26. Process Groups and Job Control](#26-process-groups-and-job-control)
- [27. Error Handling](#27-error-handling)
- [28. Testing](#28-testing)
- [29. Debugging with GDB](#29-debugging-with-gdb)
- [30. Sanitizers](#30-sanitizers)
- [31. clang-format and clang-tidy](#31-clang-format-and-clang-tidy)
- [32. CMake](#32-cmake)
- [33. Git Workflow](#33-git-workflow)
- [34. Build and Run](#34-build-and-run)
- [35. Manual Test Plan](#35-manual-test-plan)
- [36. Security and Reliability](#36-security-and-reliability)
- [37. Design Decisions](#37-design-decisions)
- [38. Known Limitations](#38-known-limitations)
- [39. Roadmap](#39-roadmap)
- [40. Portfolio Checklist](#40-portfolio-checklist)
- [41. Learning Outcomes](#41-learning-outcomes)
- [42. Troubleshooting](#42-troubleshooting)
- [43. Future Improvements](#43-future-improvements)
- [44. Glossary](#44-glossary)
- [45. Current Checkpoint](#45-current-checkpoint)

---

# 1. Overview

`myshell` is a Unix-like command-line shell written in C++20 and targeted at Linux/POSIX systems.

The project deliberately uses low-level operating-system interfaces rather than delegating execution to an existing shell. Core APIs include:

- `fork()`
- `execvp()`
- `waitpid()`
- `open()`
- `dup2()`
- `close()`
- `pipe()`
- `chdir()`
- `setenv()`
- `unsetenv()`
- POSIX signal APIs
- process-group and terminal-control APIs

The overall execution model is:

```text
User input
    |
    v
Shell REPL
    |
    v
Lexer / Parser
    |
    v
Command representation
    |
    v
Executor
    |
    +---- Built-in
    |
    +---- External command
             |
           fork()
             |
             v
           child
             |
       configure I/O
             |
          execvp()
             |
             v
       external program
```

The project starts small and grows feature-by-feature into a substantially capable interactive shell.

---

# 2. Goals

## Primary goals

- Implement a usable Unix-like shell.
- Learn Linux/POSIX process management.
- Learn file descriptors and I/O redirection.
- Learn inter-process communication with pipes.
- Understand environment inheritance.
- Understand signals and terminal/job control.
- Use modern C++ rather than putting everything in `main.cpp`.
- Maintain a professional, testable, documented codebase.

## Educational goals

By the end of the project, the developer should be able to explain:

- why `fork()` is needed
- what `exec()` actually does
- why `waitpid()` exists
- how file descriptors `0`, `1`, and `2` work
- how `dup2()` implements redirection
- how `pipe()` connects processes
- why `cd` must execute inside the shell
- how environment variables reach child processes
- how signals affect foreground jobs
- why process groups are needed for job control

---

# 3. Technology Stack

| Technology | Role |
|---|---|
| C++20 | Main implementation language |
| Linux | Target operating system |
| POSIX APIs | Process, I/O, signal, and terminal primitives |
| CMake | Build system |
| Git | Version control |
| GDB | Debugging |
| clang-format | Source formatting |
| clang-tidy | Static analysis |
| AddressSanitizer | Memory-error detection |
| UndefinedBehaviorSanitizer | Undefined-behavior detection |
| GitHub Actions | Continuous integration |
| Markdown | Documentation |

---

# 4. Environment

Development environment:

```text
OS:             Kali Linux Rolling 2026.3
Kernel:         Linux 7.0.12+kali-amd64
Architecture:   x86_64
Compiler:       G++ 15.3.0
CMake:          4.3.4
Git:            2.53.0
GDB:            17.2
clang-format:   21.1.8
clang-tidy:     LLVM 21.1.8
```

Project directory:

```text
/home/kali/projects/myshell
```

Development is performed inside a Linux virtual machine and can be accessed from VS Code using Remote SSH.

---

# 5. Architecture

The project separates user interaction, parsing, data representation, and execution.

```text
+-----------------------------+
|            Shell            |
|      REPL / interaction     |
+--------------+--------------+
               |
               v
+-----------------------------+
|           Parser            |
|       text -> structure     |
+--------------+--------------+
               |
               v
+-----------------------------+
|       Command Model         |
| Command / Pipeline /        |
| Redirection                 |
+--------------+--------------+
               |
               v
+-----------------------------+
|          Executor           |
| built-ins / fork / exec /   |
| redirection / pipes / jobs  |
+--------------+--------------+
               |
               v
+-----------------------------+
|        Linux / POSIX        |
|            kernel            |
+-----------------------------+
```

## Responsibility of each component

### `Shell`

- prints the prompt
- reads lines
- handles EOF
- invokes the parser
- invokes the executor
- keeps the interactive loop alive

### `Parser`

- turns text into structured command data
- recognizes commands and arguments
- recognizes shell operators
- identifies redirections
- identifies background execution

### `types.hpp`

Defines the structures exchanged between parser and executor.

### `Builtins`

Implements commands that must modify or inspect shell state.

### `Executor`

Handles:

- built-in execution
- child creation
- file-descriptor setup
- external execution
- waiting
- pipelines
- future job control

---

# 6. Repository Structure

Professional target:

```text
myshell/
├── .github/
│   └── workflows/
│       └── ci.yml
├── docs/
│   ├── architecture.md
│   └── internals.md
├── include/
│   └── myshell/
│       ├── builtins.hpp
│       ├── executor.hpp
│       ├── parser.hpp
│       ├── shell.hpp
│       └── types.hpp
├── src/
│   ├── main.cpp
│   ├── shell.cpp
│   ├── parser.cpp
│   ├── executor.cpp
│   └── builtins.cpp
├── tests/
│   ├── parser_tests.cpp
│   └── integration/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── .gitignore
└── .clang-format
```

Later job-control code may add:

```text
include/myshell/jobs.hpp
src/jobs.cpp
```

---

# 7. Core Data Model

The central model is:

```cpp
struct Command
{
    std::string program;
    std::vector<std::string> arguments;
};
```

For:

```bash
ls -la /tmp
```

the model is approximately:

```text
program:
    ls

arguments:
    -la
    /tmp
```

Redirections are represented as:

```cpp
struct Redirection
{
    enum class Type
    {
        Input,
        Output,
        Append
    };

    Type type;
    std::string filename;
};
```

A pipeline is:

```cpp
struct Pipeline
{
    std::vector<Command> commands;
    std::vector<Redirection> redirections;
    bool background = false;
};
```

This design deliberately keeps parsing separate from execution.

That allows parser tests to operate without creating real processes.

---

# 8. Shell REPL

REPL means:

```text
Read
Evaluate
Print
Loop
```

The shell does:

```text
print prompt
    |
read input
    |
parse
    |
execute
    |
repeat
```

Typical interaction:

```text
myshell$ pwd
/home/kali/projects/myshell
myshell$ echo hello
hello
myshell$
```

EOF is handled by detecting failure from `std::getline()`.

---

# 9. Parser

The current parser is intentionally simple and whitespace-based.

It recognizes operators such as:

```text
|
>
>>
<
&
```

For example:

```bash
echo hello > file.txt
```

is converted into:

```text
Command
    program = echo
    arguments = [hello]

Redirection
    type = Output
    filename = file.txt
```

## Current limitation

The parser is not yet a complete shell lexer.

It does not correctly model all cases involving:

- quotes
- escapes
- variable expansion
- command substitution
- compound commands
- advanced redirection

The planned evolution is:

```text
raw input
    |
    v
lexer
    |
    v
tokens
    |
    v
parser
    |
    v
AST / command model
```

This is preferable to continually adding special cases to a whitespace parser.

---

# 10. Built-ins

Current built-ins:

```text
cd
pwd
echo
export
unset
exit
help
```

## Why `cd` is special

If the shell did:

```text
shell
  |
 fork()
  |
 child
  |
 chdir("/tmp")
  |
 exit()
```

only the child changes directory.

The shell remains in its original directory.

Therefore:

```text
shell
  |
  +-- chdir("/tmp")
```

must happen inside the shell process.

This is the key reason built-ins are not treated exactly like external commands.

## `exit`

`exit` terminates the shell process.

## `pwd`

Reports the shell's current working directory.

## `echo`

Prints its arguments.

## `export`

Changes the shell environment using `setenv()`.

## `unset`

Removes variables using `unsetenv()`.

## `help`

Displays supported built-ins and usage.

---

# 11. Processes

A process is a running program instance together with its execution state and operating-system resources.

The shell normally has this relationship:

```text
shell process
     |
     +---- child process
```

The child can then become:

```text
ls
```

or:

```text
grep
```

or another external program.

The three core operations are:

```text
fork()  -> create process
exec()  -> replace process image
waitpid() -> wait for child
```

These are conceptually separate operations.

---

# 12. fork()

`fork()` creates a child process.

Typical pattern:

```cpp
pid_t pid = fork();

if (pid < 0)
{
    // error
}
else if (pid == 0)
{
    // child
}
else
{
    // parent
}
```

Meaning:

```text
pid < 0
    fork failed

pid == 0
    this code is executing in child

pid > 0
    this code is executing in parent
```

Important:

> `fork()` creates a process; it does not launch a new executable.

---

# 13. execvp()

The child initially has the shell's process image.

It then calls:

```cpp
execvp(command.program.c_str(), argv.data());
```

If successful, the current process image is replaced.

Conceptually:

```text
child containing myshell
        |
      execvp()
        |
        v
child containing ls
```

The process does not get a new PID merely because `execvp()` was called.

The existing process is transformed into the requested program.

Important rule:

> If `execvp()` returns, execution failed.

Therefore the child must report the error and terminate rather than continuing to execute shell-parent logic.

---

# 14. waitpid()

The parent normally waits for a foreground command:

```cpp
waitpid(pid, &status, 0);
```

Flow:

```text
parent
  |
  +-- waitpid()
  |
  |      child running
  |          |
  |          v
  |       finishes
  |
  v
parent continues
```

The status can be inspected with:

```cpp
WIFEXITED(status)
WEXITSTATUS(status)
```

A foreground shell should generally wait before printing the next prompt.

Background execution will later require different waiting behavior.

---

# 15. PATH

A user can type:

```bash
ls
```

instead of:

```bash
/bin/ls
```

because the command is normally resolved using the `PATH` environment variable.

Example:

```text
PATH=/usr/local/bin:/usr/bin:/bin
```

`execvp()` performs PATH searching.

This means the shell can currently use the operating system's standard executable lookup behavior rather than implementing a custom search routine.

---

# 16. File Descriptors

Unix processes normally start with:

```text
0 = stdin
1 = stdout
2 = stderr
```

Typical state:

```text
stdin
  |
  v
keyboard

stdout
  |
  v
terminal

stderr
  |
  v
terminal
```

The shell manipulates these descriptors to implement redirection and pipelines.

This is one of the most important concepts in Unix systems programming.

---

# 17. Output Redirection

Command:

```bash
ls > output.txt
```

means:

```text
ls stdout
    |
    v
output.txt
```

The child should perform:

```cpp
int fd = open(
    filename.c_str(),
    O_WRONLY | O_CREAT | O_TRUNC,
    0644
);
```

Then:

```cpp
dup2(fd, STDOUT_FILENO);
close(fd);
```

## `open()`

Flags:

```text
O_WRONLY
    open for writing

O_CREAT
    create the file if necessary

O_TRUNC
    remove existing contents
```

`0644` is the creation mode before the process umask is applied.

## `dup2()`

```cpp
dup2(fd, STDOUT_FILENO);
```

makes descriptor `1` refer to the same open file as `fd`.

Then:

```cpp
close(fd);
```

removes the extra descriptor.

The result is:

```text
stdout (1)
    |
    v
output.txt
```

## Critical rule

Redirection for an external command must happen in the child.

Correct:

```text
parent
  |
fork()
  |
  +-- child
       |
       +-- open()
       +-- dup2()
       +-- close()
       +-- execvp()
```

If the parent redirects its own stdout, the shell prompt itself can be redirected.

---

# 18. Append Redirection

Command:

```bash
echo first > file.txt
echo second >> file.txt
```

uses:

```text
>   overwrite
>>  append
```

Append uses:

```cpp
O_WRONLY | O_CREAT | O_APPEND
```

instead of:

```cpp
O_WRONLY | O_CREAT | O_TRUNC
```

Expected:

```text
first
second
```

---

# 19. Input Redirection

Command:

```bash
cat < file.txt
```

means:

```text
file.txt
   |
   v
stdin (0)
   |
   v
cat
```

Implementation:

```cpp
int fd = open(filename.c_str(), O_RDONLY);

dup2(fd, STDIN_FILENO);

close(fd);
```

Now when `cat` reads descriptor `0`, it reads from the file.

This demonstrates a general Unix principle:

> Programs usually do not need to know whether stdin/stdout is a terminal, file, or pipe. They simply read and write file descriptors.

---

# 20. Built-in Redirection

External commands get a child process, so their descriptors can be modified safely inside that child.

Built-ins are different.

For:

```bash
echo hello > file.txt
```

`echo` executes inside the shell process.

Therefore the shell must save and restore its descriptors.

Conceptually:

```cpp
int saved_stdout = dup(STDOUT_FILENO);

dup2(file_fd, STDOUT_FILENO);

execute_builtin(command);

dup2(saved_stdout, STDOUT_FILENO);
close(saved_stdout);
```

Flow:

```text
save shell stdout
        |
        v
redirect stdout
        |
        v
run builtin
        |
        v
restore stdout
```

The same principle applies to stdin and, when supported, stderr.

---

# 21. Pipes

A pipe connects one process's output to another process's input.

Example:

```bash
ls | grep cpp
```

Desired flow:

```text
ls
 |
 | stdout
 v
+-------+
| pipe  |
+-------+
   |
   | stdin
   v
grep
```

Creation:

```cpp
int pipefd[2];
pipe(pipefd);
```

Convention:

```text
pipefd[0] = read end
pipefd[1] = write end
```

Typical implementation:

1. create pipe
2. fork first child
3. connect first child's stdout to write end
4. fork second child
5. connect second child's stdin to read end
6. close unused descriptors
7. execute commands
8. wait for foreground children

For:

```bash
a | b | c
```

there are two pipes:

```text
a -> pipe1 -> b -> pipe2 -> c
```

Closing unused descriptors is critical. A reader may wait for EOF forever if a write end remains open in another process.

---

# 22. Environment Variables

Environment variables are name/value strings inherited by child processes.

Useful shell operations:

```bash
export NAME=value
unset NAME
```

Relevant APIs:

```cpp
getenv()
setenv()
unsetenv()
```

Example:

```cpp
setenv("NAME", "value", 1);
```

The shell changes its own environment.

A child created with `fork()` inherits that environment, and the environment is available to the program launched by `execvp()`.

This is why environment-changing commands must be shell built-ins.

---

# 23. Quoting and Escaping

A real shell must distinguish:

```bash
echo hello world
```

from:

```bash
echo "hello world"
```

The second form contains one argument containing a space.

It must also handle:

```bash
echo 'hello world'
```

and:

```bash
echo hello\ world
```

A future lexer should understand token boundaries explicitly.

## Single quotes

Generally preserve contents literally.

## Double quotes

Preserve whitespace while still allowing shell-specific expansion/escape rules.

## Backslash

Can escape special characters.

Example:

```bash
echo hello\ world
```

should produce one argument:

```text
hello world
```

The current whitespace parser should eventually be replaced by a dedicated lexer.

---

# 24. Background Processes

Command:

```bash
sleep 10 &
```

should return the prompt without waiting ten seconds.

Foreground:

```text
shell
 |
 +-- child
 |
 wait
 |
 prompt
```

Background:

```text
shell
 |
 +-- child
 |
 +-- prompt immediately
```

Background execution introduces zombie-process management.

The shell will eventually need mechanisms such as:

```text
waitpid(..., WNOHANG)
SIGCHLD
job table
process groups
```

---

# 25. Signals

Important signals for an interactive shell include:

```text
SIGINT
SIGTERM
SIGTSTP
SIGCHLD
```

Terminal shortcuts commonly generate:

```text
Ctrl+C -> SIGINT
Ctrl+Z -> SIGTSTP
```

When the user runs:

```bash
sleep 100
```

and presses Ctrl+C, the desired architecture is:

```text
terminal
   |
   v
foreground process group
   |
   v
sleep
```

The shell itself should remain alive.

This is why job control is more complicated than simply installing one signal handler.

---

# 26. Process Groups and Job Control

Pipelines are naturally treated as jobs.

Example:

```bash
cat file | grep hello | less
```

Conceptually:

```text
Job
 |
 +-- cat
 +-- grep
 +-- less
```

A shell can place related processes into a process group.

The terminal can then give foreground control to that group.

This provides the foundation for:

```bash
jobs
fg
bg
```

and correct behavior for:

```text
Ctrl+C
Ctrl+Z
```

Job control is one of the most advanced parts of the project and should be implemented only after basic process execution and pipes are stable.

---

# 27. Error Handling

A shell must survive bad commands.

Examples:

```bash
cd /does/not/exist
cat < missing.txt
unknown_command
```

The shell should report the error and continue.

Many POSIX functions indicate failure by returning a negative value or `-1` and setting `errno`.

Typical reporting:

```cpp
perror("myshell: fork");
```

or:

```cpp
std::strerror(errno)
```

## Child errors

If:

```cpp
execvp(...)
```

fails, the child should report the failure and exit with an appropriate non-zero status.

It should not return into normal shell code.

---

# 28. Testing

A professional project should test at multiple levels.

## Unit tests

Test parser behavior without creating processes.

Example:

```text
Input:
echo hello > output.txt

Expected:
program = echo
argument = hello
redirection type = Output
filename = output.txt
```

## Integration tests

Run the real executable.

Examples:

```bash
echo hello
pwd
echo first > file.txt
cat file.txt
```

## Failure tests

Examples:

```bash
cat < missing.txt
cd /missing
unknown_command
```

## Regression tests

Every important bug should become a test so it cannot silently return.

---

# 29. Debugging with GDB

Debug build:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Start:

```bash
gdb ./build/myshell
```

Useful commands:

```gdb
break main
run
next
step
continue
print variable
bt
```

For process debugging:

```gdb
set follow-fork-mode child
```

can make GDB follow the child after `fork()`.

When debugging this project, always know whether you are examining:

```text
parent shell
```

or:

```text
child command
```

because their responsibilities are different.

---

# 30. Sanitizers

## AddressSanitizer

Useful for finding:

- buffer overflows
- use-after-free
- invalid memory access
- related memory bugs

Typical flag:

```text
-fsanitize=address
```

## UndefinedBehaviorSanitizer

Useful for runtime undefined behavior.

Typical flag:

```text
-fsanitize=undefined
```

A mature CI configuration should build and test sanitizer configurations.

---

# 31. clang-format and clang-tidy

## clang-format

Maintains consistent formatting.

Example:

```bash
clang-format -i src/*.cpp include/myshell/*.hpp
```

The repository should eventually contain `.clang-format`.

## clang-tidy

Performs static analysis and can identify:

- suspicious constructs
- readability issues
- possible bugs
- opportunities for modern C++

Warnings should be understood before being changed rather than blindly silenced.

---

# 32. CMake

The project uses CMake.

Core configuration:

```cmake
cmake_minimum_required(VERSION 3.20)

project(myshell
    VERSION 0.1.0
    DESCRIPTION "A Unix shell implemented in C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

The final executable should include:

```text
src/main.cpp
src/shell.cpp
src/parser.cpp
src/executor.cpp
src/builtins.cpp
```

Generated files remain in:

```text
build/
```

and are ignored by Git.

---

# 33. Git Workflow

The repository was initialized with:

```text
3c90be4 chore: initialize myshell project
```

Recommended commit style:

```text
feat: add shell REPL
feat: add builtin commands
feat: implement external command execution
feat: add output redirection
feat: add append redirection
feat: add input redirection
feat: implement pipelines
feat: add environment expansion
test: add parser tests
test: add integration tests
docs: document architecture
ci: add GitHub Actions workflow
```

Typical workflow:

```bash
git status
git diff
git add .
git commit -m "feat: add output redirection"
```

Before committing, build:

```bash
cmake --build build
```

Once tests exist:

```bash
ctest --test-dir build
```

The goal is a clean, understandable history where commits represent logical improvements.

---

# 34. Build and Run

From:

```text
/home/kali/projects/myshell
```

Configure:

```bash
cmake -S . -B build
```

Build:

```bash
cmake --build build
```

Run:

```bash
./build/myshell
```

One-shot non-interactive test:

```bash
printf 'echo hello\nexit\n' | ./build/myshell
```

This is especially useful for integration testing and CI.

---

# 35. Manual Test Plan

## REPL

```bash
./build/myshell
```

Then:

```bash
echo hello
pwd
help
exit
```

## External programs

```bash
ls
whoami
date
```

## Directory changes

```bash
pwd
cd /tmp
pwd
```

## Output redirection

```bash
echo hello > file.txt
cat file.txt
```

Expected:

```text
hello
```

## Overwrite

```bash
echo first > file.txt
echo second > file.txt
cat file.txt
```

Expected:

```text
second
```

## Append

```bash
echo first > file.txt
echo second >> file.txt
cat file.txt
```

Expected:

```text
first
second
```

## Input

```bash
cat < file.txt
```

## Missing file

```bash
cat < missing.txt
```

Expected:

- an error message
- shell remains alive

## Future pipeline

```bash
ls | grep cpp
```

Then:

```bash
ls | grep cpp | wc -l
```

## Future environment

```bash
export NAME=myshell
printenv NAME
unset NAME
```

## Future signal handling

```bash
sleep 100
```

Press:

```text
Ctrl+C
```

Expected:

```text
sleep terminates
shell survives
```

---

# 36. Security and Reliability

Even though this is an educational project, it should use production-minded habits.

## Do not use `system()` for external command execution

Avoid:

```cpp
system(user_input.c_str());
```

That delegates execution back to another shell.

The project instead uses:

```text
fork()
execvp()
```

## Validate parser state

Malformed input should not cause crashes or undefined behavior.

Examples:

```text
>
<
|
```

must be rejected or handled intentionally.

## Check system calls

Review failure handling for:

```text
fork
execvp
waitpid
open
dup2
close
pipe
chdir
setenv
unsetenv
```

## Descriptor ownership

Every opened descriptor should have a clear owner and lifetime.

This becomes especially important with multiple pipes.

---

# 37. Design Decisions

## Why C++20?

It provides modern language and library features while still allowing direct POSIX calls.

## Why POSIX APIs?

The project's educational purpose is to understand the operating-system boundary directly.

## Why separate classes?

Putting everything into `main.cpp` makes testing and extension difficult.

The project instead separates:

```text
Shell
Parser
Executor
Builtins
```

## Why a command data model?

It lets the parser produce structured data without immediately executing anything.

That enables:

- parser unit tests
- cleaner execution
- future AST support
- easier feature additions

---

# 38. Known Limitations

Current/future limitations include:

- simplistic whitespace-based parser
- incomplete shell grammar
- no complete quoting system yet
- no escaping system yet
- no command substitution
- no full variable expansion
- no `&&` / `||`
- no command separators
- no glob expansion
- no advanced file-descriptor syntax
- no heredocs
- no full shell scripting language
- no complete job-control implementation

These limitations are intentional and belong to the roadmap.

---

# 39. Roadmap

## Level 1 — Foundation

1. Project setup
2. CMake
3. First executable
4. Git workflow
5. Shell REPL

## Level 2 — Built-ins

6. Built-in commands
7. Command parsing

## Level 3 — Process Execution

8. `fork()`
9. `exec()`
10. `waitpid()`
11. External commands
12. PATH resolution

## Level 4 — I/O

13. stdout
14. stderr
15. `>` output redirection
16. `>>` append redirection
17. `<` input redirection

## Level 5 — Pipes

18. `pipe()`
19. first pipeline
20. multiple pipelines

## Level 6 — Environment

21. environment variables
22. `export`
23. `unset`
24. variable expansion

## Level 7 — Shell Language

25. quoting
26. single quotes
27. double quotes
28. escaping

## Level 8 — Command Logic

29. separators
30. `&&`
31. `||`

## Level 9 — Background Execution

32. `&`
33. process groups
34. background process management

## Level 10 — Signals

35. signal handling
36. Ctrl+C
37. Ctrl+Z

## Level 11 — Job Control

38. jobs
39. `fg`
40. `bg`

## Level 12 — Refactoring

41. refactor parser
42. AST / command representation
43. separate execution layer
44. centralized error handling

## Level 13 — Quality

45. unit tests
46. integration tests
47. clang-format
48. clang-tidy
49. GDB
50. AddressSanitizer
51. UndefinedBehaviorSanitizer

## Level 14 — Documentation and Release

52. README
53. architecture documentation
54. internals documentation
55. GitHub Actions CI
56. release workflow
57. demo/screenshots
58. version `1.0.0`

---

# 40. Portfolio Checklist

## Build

- [ ] Clean CMake configuration
- [ ] Debug build
- [ ] Release build
- [ ] Reproducible build instructions
- [ ] Generated files excluded from Git

## Features

- [ ] REPL
- [ ] `cd`
- [ ] `pwd`
- [ ] `echo`
- [ ] `export`
- [ ] `unset`
- [ ] `exit`
- [ ] `help`
- [ ] external commands
- [ ] PATH execution
- [ ] `>`
- [ ] `>>`
- [ ] `<`
- [ ] pipes
- [ ] environment expansion
- [ ] quoting
- [ ] escaping
- [ ] background jobs
- [ ] signals
- [ ] process groups
- [ ] job control

## Quality

- [ ] Unit tests
- [ ] Integration tests
- [ ] Failure tests
- [ ] Regression tests
- [ ] clang-format
- [ ] clang-tidy
- [ ] GDB workflow
- [ ] ASan
- [ ] UBSan

## Documentation

- [ ] README
- [ ] architecture.md
- [ ] internals.md
- [ ] build instructions
- [ ] usage examples
- [ ] known limitations
- [ ] roadmap
- [ ] contribution instructions

## CI

- [ ] Linux build
- [ ] tests
- [ ] formatting check
- [ ] static analysis
- [ ] sanitizer build

## Release

- [ ] version number
- [ ] changelog
- [ ] release notes
- [ ] Git tag
- [ ] demo
- [ ] screenshots
- [ ] clean Git history

---

# 41. Learning Outcomes

The final project should make these statements understandable rather than memorized.

### `fork()`

Creates a new process.

### `exec()`

Replaces the current process image with another program.

### `waitpid()`

Waits for a child and retrieves its termination status.

### File descriptors

```text
0 = stdin
1 = stdout
2 = stderr
```

### `dup2()`

Redirects a file descriptor by making the destination descriptor refer to the same open file description as the source.

### `pipe()`

Creates a kernel-managed communication channel with a read end and a write end.

### Built-ins

Commands such as `cd` must execute in the shell process because they modify shell state.

### Environment inheritance

Child processes inherit the environment from their parent and pass it to programs executed through `exec`.

### Process groups

Group related processes so the terminal can manage an entire foreground/background job.

---

# 42. Troubleshooting

## CMake error

Check:

```bash
find src include -maxdepth 3 -type f
```

Then compare the result with `CMakeLists.txt`.

## Compiler error

Run:

```bash
cmake --build build
```

Focus on the first meaningful error. Later errors may be consequences of it.

## `execvp()` failure

Check:

```bash
which ls
which cat
echo "$PATH"
```

The shell should report a useful message rather than crash.

## Redirection failure

For:

```bash
echo hello > file.txt
```

verify the sequence:

```text
fork
 |
child
 |
open
 |
dup2
 |
close
 |
execvp
```

Redirection must not permanently alter the parent shell's stdout.

## Shell output disappears after builtin redirection

The shell probably redirected its own stdout without restoring it.

Correct sequence:

```text
save
redirect
execute builtin
restore
```

## Pipe hangs

Check descriptor closing.

If a process still has a pipe write end open, another process may never observe EOF.

---

# 43. Future Improvements

After the core shell is stable, possible extensions include:

## Lexer

A real tokenizer with explicit token types.

## AST

A more formal representation of shell grammar.

Example:

```text
Pipeline
 |
 +-- Command
 +-- Command
 +-- Command
```

## Advanced redirection

Potential support:

```bash
command 2> errors.txt
command > out.txt 2> errors.txt
command > out.txt 2>&1
```

## Heredocs

```bash
cat <<EOF
hello
EOF
```

## Command substitution

```bash
echo "$(pwd)"
```

## Globbing

```bash
ls *.cpp
```

## Shell scripting

Potentially:

```bash
if
for
while
case
```

This should be treated as a much larger language-design stage.

---

# 44. Glossary

**Shell** — A command interpreter that reads commands and launches or controls programs.

**Process** — A running instance of a program with operating-system-managed state.

**PID** — Process ID.

**Parent process** — A process that created another process.

**Child process** — A process created by another process.

**Process image** — The executable program and runtime state represented by a process.

**File descriptor** — An integer handle referring to an open I/O resource.

**stdin** — Standard input, descriptor `0`.

**stdout** — Standard output, descriptor `1`.

**stderr** — Standard error, descriptor `2`.

**Redirection** — Changing where a standard descriptor reads from or writes to.

**Pipe** — A kernel-provided communication channel between processes.

**Environment** — Name/value strings inherited by child processes.

**Signal** — An asynchronous notification delivered to a process or process group.

**Process group** — A collection of processes managed together for terminal/job control.

**Foreground job** — The job currently associated with the terminal's foreground process group.

**Background job** — A job running without foreground terminal ownership.

**REPL** — Read-Eval-Print Loop.

**POSIX** — Standards defining common Unix-like operating-system interfaces and behavior.

---

# 45. Current Checkpoint

The project currently has:

- C++20 project structure
- CMake build
- Git repository
- shell REPL
- command representation
- parser
- built-in commands
- `fork()`
- `execvp()`
- `waitpid()`
- external command execution
- PATH-based execution through `execvp()`
- redirection data structures
- parser recognition for:
  - `>`
  - `>>`
  - `<`
  - `|`
  - `&`

The immediate implementation milestone is **output redirection (`>`)**.

## First target

Make this work:

```bash
echo hello > file.txt
cat file.txt
```

Expected:

```text
hello
```

Then verify overwrite:

```bash
echo first > file.txt
echo second > file.txt
cat file.txt
```

Expected:

```text
second
```

Then implement append:

```bash
echo first > file.txt
echo second >> file.txt
cat file.txt
```

Expected:

```text
first
second
```

Then input:

```bash
cat < file.txt
```

For external commands, redirection belongs in the child process.

The clean future abstraction is:

```cpp
bool apply_redirections(const Pipeline& pipeline);
```

Its responsibilities:

1. iterate over `pipeline.redirections`
2. determine the redirection type
3. call `open()`
4. call `dup2()`
5. call `close()`
6. report errors

`executor.cpp` needs:

```cpp
#include <fcntl.h>
```

`unistd.h` already provides the declarations needed for `dup2()` and `close()`.

After redirection, the next major milestone is:

```text
pipe()
```

followed by pipelines, environment expansion, proper lexical analysis, background processes, signals, process groups, job control, automated testing, sanitizers, static analysis, documentation, CI, and release.

---

# Final Definition of Done

`myshell` is portfolio-ready when:

1. It builds from a clean checkout.
2. The documented commands reproduce the build.
3. Core functionality has automated tests.
4. Error paths have tests.
5. Sanitizers pass.
6. Static analysis is reviewed.
7. Formatting is consistent.
8. CI builds and tests the project.
9. Architecture documentation matches the implementation.
10. README contains a concise demo.
11. Git history is understandable.
12. The project has a tagged release.
13. Known limitations are documented.
14. The developer can explain the implementation without treating it as a black box.

The real achievement is not merely having a shell executable.

It is understanding the complete path:

```text
user input
    |
    v
lexer / parser
    |
    v
command representation
    |
    v
executor
    |
    +---- builtin
    |
    +---- fork()
             |
             v
           child
             |
       redirection / pipe
             |
          execvp()
             |
             v
       external program
             |
          waitpid()
             |
             v
        shell prompt
```

That is the systems-programming knowledge this project is designed to build.
