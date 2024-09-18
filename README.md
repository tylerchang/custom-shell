# Custom Linux Shell

## Overview
This system programming project is a custom-built shell for Linux systems, implemented in C. Notable features of the project include:
### Only System Calls
Only used direct system calls (`syscall()`) instead of wrapper library functions for core operations
### Dynamic Input Handling
  - Supports arbitrarily long input commands and dynamically resizes input buffer as needed
  - Utilizes `mmap()` for efficient memory allocation and `munmap()` for deallocation
### Process Management
  - Uses `clone()` system call for process creation and command execution which utilizes Linux-specific process management techniques
  - Implements proper child process waiting and resource cleanup
### Signal Handling
  - Implements custom signal handling for SIGINT (Ctrl+C)
  - Shell ignores SIGINT
  - Child processes reset to default SIGINT handling

## Features
### Command Execution
- Executes standard Unix/Linux commands (e.g., ls, cat, grep)
- Supports command-line arguments
### Built-in Commands
- `cd`: Change directory
  - Usage: `cd [directory]`
  - Supports changing to home directory when used without arguments
- `exit`: Terminate the shell
  - Usage: `exit`

### Error Handling
- Comprehensive error checking and reporting for system calls
- User-friendly error messages displayed for various scenarios

## Limitations and Future Improvements
- Does not implement command history or tab completion
- Limited to a predefined set of environment variables

## Building and Running
`make` will generate `shell_executable` which can be executed with `./shell_executable` to start the shell.
