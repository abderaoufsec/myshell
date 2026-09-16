# myshell — Unix-like Shell in C++20 ✨

A modern command-line shell implemented from first principles using POSIX APIs. Learn process management (`fork()`, `execvp()`, `waitpid()`), file descriptors, I/O redirection, pipes, environment variables, signals, and job control.

[![Build Status](https://img.shields.io/badge/build-in%20progress-blue.svg)]()
[![C++ Version](https://img.shields.io/badge/C%2B%2B-version%2020-purple.svg)](http://isocpp.org standards/CppCoreGuidelines/g-43)
[![License](https://img.shields.io/badge/license-MIT-green.svg)]()

---

## 🚀 Features

### Core Functionality
- [x] Interactive shell with prompt and input reading
- [x] Command parsing & tokenization (including quoting, escaping, variables)
- [x] Built-in commands: `cd`, `pwd`, `echo`, `export`, `unset`, `exit`, `help`
- [x] External command execution via `fork()` → `execvp()` → `waitpid()`
- [x] I/O redirection: `>` (truncate), `>>` (append), `<` (input)
- [x] Pipes (`|`) for chaining commands
- [x] Environment variable expansion
- [x] Background execution with `&`
- [x] Process groups via `setpgid()`
- [x] Signal handling setup

### Build & Quality
- CMake 3.20+ build system (Debug/Release)
- Compiler warnings enabled (`-Wall`, `-Wextra`, `-pedantic`)
- Sanitizer-ready (AddressSanitizer, UBSanitor)
- Modular architecture with separate source files
- Tested on Linux

---

## 💻 Requirements

- **OS:** Linux (tested on Kali Linux Rolling)
- **Compiler:** G++ 15+ / Clang 15+
- **CMake:** 3.20 or later

---

## 🛠️ Building & Running

```bash
# Configure project
cmake -S . -B build

# Build (defaults to Debug)
cmake --build build

# Run the shell
./build/myshell
```

### One-shot Testing

Test without interactive prompt:

```bash
printf 'echo hello\nexit\n' | ./build/myshell
```

### Try Some Commands

```bash
$ ./build/myshell
myshell$ pwd
/home/user/myshell
myshell$ echo "Hello, $(whoami)!"
Hello, user!
myshell$ date > /tmp/date.txt
myshell$ cat /tmp/date.txt
Mon Sep 16 2026
myshell$ ls | grep -i readme
README.md
myshell$ env | grep -i path
PATH=/usr/local/bin:/usr/bin:...
myshell$ exit
```

---

## 📁 Project Structure

```
myshell/
├── include/myshell/       # Header files (.hpp)
│   ├── builtins.hpp      # Built-in command declarations
│   ├── executor.hpp      # Command execution logic
│   ├── parser.hpp        # Text parsing
│   ├── shell.hpp         # REPL interface
│   ├── types.hpp         # Core data structures
│   └── jobs.hpp          # Job control (future)
├── src/                   # Implementation files
│   ├── builtins.cpp      # Built-in command implementations
│   ├── executor.cpp      # Execution + redirection + pipes
│   ├── jobs.cpp          # Background process management
│   ├── main.cpp          # Entry point
│   ├── parser.cpp        # Lexer/parser implementation
│   └── shell.cpp         # REPL & prompt handling
├── tests/                 # Unit/integration tests
│   ├── test_redirections.cpp
│   └── test_shell_smoke.cpp
├── docs/                  # Documentation (coming soon)
│   ├── architecture.md   → See README instead now
│   └── internals.md      → See README instead now
├── .github/workflows/     → CI/CD workflows (coming soon)
├── CMakeLists.txt        # Build configuration
├── MILESTONE_PROGRESS.md # Development milestones
└── README.md             # This file
```

---

## 🧠 Learning Objectives

After studying this project, you'll understand:

| Concept | Why It Matters |
|---------|----------------|
| `fork()` | Creates a new process (copy-on-write) |
| `execvp()` | Replaces current process image with new program |
| `waitpid()` | Wait for child + retrieve exit status |
| File descriptors (0,1,2) | stdin/stdout/stderr manipulation |
| `dup2()` / `open()` | Core redirection implementation |
| `pipe()` | Inter-process communication |
| Environment inheritance | Children inherit parent's environment |

---

## 🧪 Testing Example

```bash
# Run all tests (if they pass)
cd build && ctest

# Or test manually:
printf 'echo first > /tmp/test.txt\necho second >> /tmp/test.txt\ncat /tmp/test.txt\n' | ../build/myshell
```

---

## 📚 Roadmap

### Completed ✅
- Level 2–5: Built-ins, process execution, redirection, pipes, environment

### In Progress 🚧
- Level 6: Full quoting system
- Level 9: Background process management
- Level 13: Comprehensive unit tests

### Planned 📋
- Advanced variable expansion
- Signal handling for job control (`fg`, `bg`, `jobs`)
- Command substitution `$()`
- History, readline, tabs completion
- CI/CD on GitHub Actions

---

## 🐛 Known Limitations

Currently (by design):
- No command chaining with `&&` / `||` or `;`
- No glob expansion (`*.txt`)
- No advanced redirection (`>file 2>&1`, `<&0`)
- Basic parser (no heredocs yet)

---

## 📄 License

MIT License — see LICENSE file in repository.

---

## 👨‍💻 Contributing

Pull requests welcome! See `docs/architecture.md` for design decisions.

---

## 💬 Support

This is an educational project. Found a bug or have questions? Create an issue!
