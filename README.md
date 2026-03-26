# Shell Script Interpreter

This project was developed for the Operating Systems course (Academic Year 2024/2025) at Universidad Carlos III de Madrid. It includes a custom shell script interpreter (`scripter`) and a search utility (`mygrep`) written in C. 
## Project Components

### 1. Scripter (`scripter.c`)
This program simulates a command interpreter that reads a script file and executes each line as if it were a terminal instruction. It is capable of handling complex command-line structures. 

**Key Features:**
* **Unlimited Pipes:** Connects an unlimited sequence of commands using pipes (`|`).
* **Redirections:** Supports standard input (`<`), standard output (`>`), and standard error (`!>`) redirections. The program was specifically modified to successfully process multiple redirections within the same command.
* **Background Execution:** Commands ending with `&` are executed in the background. The shell uses a `SIGCHLD` signal handler to collect the status of child processes when they finish, preventing them from becoming zombie processes.
* **Argument Formatting:** Includes a function to automatically remove quotes from arguments so they are processed correctly.

### 2. MyGrep (`mygrep.c`)
A custom implementation of the standard `grep` command. It searches for a specific text string within a given text file and prints the lines where the string appears to the standard output.

## Compilation

A `Makefile` is provided to easily compile and clean the project.

* **Compile everything:** Run `make` to compile both `scripter` and `mygrep`.
* **Compile mygrep only:** Run `make mygrep`.
* **Clean directory:** Run `make clean` to remove all generated object files (`.o`) and executables.

## Usage

### Running Scripter

To run the shell interpreter, pass the script file as an argument:
`./scripter <script_file>`

**Important:** The script file must start exactly with the following header on its first line, otherwise the program will throw an error and terminate:
`## Script de SSOO`

### Running MyGrep

To run the custom grep utility, provide the file path and the string you want to search for:
`./mygrep <file_path> <search_string>`

If the search string contains multiple words, make sure to enclose it in quotes (e.g., `"sistemas operativos"`). If the string is not found, the program will output `"<search_string>" not found.`.
