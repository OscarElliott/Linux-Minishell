# Linux-Shell
Linux-Shell is a simple UNIX shell implemented in C, providing basic command-line functionality and supporting background execution, file operations (delete, rename, copy).


## Features
- **Command Execution:** Execute commands in the shell, including external commands and built-in commands like `cd`.

- **Background Execution:** Run commands in the background with threads using the `&` symbol.

- **File Operations:**
  - `rm`: Delete a file or directory.
  - `mv`: Rename a file or directory.
  - `cp`: Copy a file or directory.

- **Job Control:** Track and manage background jobs.

## Getting Started

### Prerequisites

- GCC compiler
- UNIX-like operating system

### Building & Running

- make run

### Usage

#### Basic Commands

- Execute a command: ls, echo, etc.
- Change directory: cd <directory>.
- Run a command in the background: <command> &.

#### File Operations
Delete a file or directory
    rm <filename>

Rename a file or directory
    mv "old name.txt" new_name.txt

Copy a file or directory
    cp source.txt "destination.txt"

## Contributors

- [Oscar Elliott](https://github.com/OscarElliott)

## Liscence 
This project is MIT licensed. View full [License](https://en.wikipedia.org/wiki/MIT_License)
