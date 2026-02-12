# SysY Compiler

![Standard](https://img.shields.io/badge/standard-C%2B%2B23-blue?logo=c%2B%2B&logoColor=white)
![Build](https://img.shields.io/badge/build-passing-brightgreen)
![Language](https://img.shields.io/badge/language-C%2B%2B-blue)
![Modules](https://img.shields.io/badge/modules-C%2B%2B20-blue?logo=c%2B%2B&logoColor=white)

A SysY compiler implementation targeting Koopa IR and RISC-V Assembly. Developed for the PKU Compile Principle Laboratory.

## Features

- **Frontend**: Lexical and Syntactic analysis using Flex & Bison. Generates an Abstract Syntax Tree (AST).
- **Middle-end**: Generates Koopa IR (Intermediate Representation) from AST.
- **Backend**: Generates RISC-V assembly from Koopa IR.
- **Modern C++**: Built with **C++20 Modules** and C++23 standards for better modularity and compilation speed.
- **Dependencies**: Uses `fmt` library for formatting (self-contained, fetched via CMake).

## Build Environments

This project is designed to run in a Docker environment since LLVM and other dependencies can be tricky to configure locally. You have two ways to build the project.

### Method 1: VS Code Dev Containers (Recommended)

1. Ensure you have Docker and VS Code installed with the **Dev Containers** extension.
2. Open this project folder in VS Code.
3. When prompted (or via the Command Palette `F1`), select **Dev Containers: Reopen in Container**.
4. Once inside the container, the environment is pre-configured. You can build via the terminal:

   ```bash
   make
   ```

### Method 2: Manual Docker Shell

If you are not using VS Code Dev Containers, you can use the provided `Makefile` to enter the Docker environment.

1. Enter the Docker container:
   ```bash
   make shell
   ```

2. **Note**: The environment requires `Ninja` for the build system tailored in `Makefile`. Since `sudo` is not available, you should **manually download the Ninja binary**:

3. Build the project using `make`:
   ```bash
   make
   ```
   *This command uses `cmake` with the `Ninja` generator.*

## Usage

The compiler requires specific mandatory arguments to function correctly. The basic command structure is:

```bash
./compiler -[mode] <input_file> -o <output_file>
```

### Mandatory Mode Options
One of the following modes **must** be specified to generate output:

- `-koopa`: Compile SysY source to **Koopa IR**.
- `-riscv`: Compile SysY source to **RISC-V assembly**.
- `-perf`: Compile with performance optimizations enabled.

### Examples

**Compile to Koopa IR:**
```bash
./cmake-build/compiler -koopa test.c -o test.koopa
```

**Compile to RISC-V Assembly:**
```bash
./cmake-build/compiler -riscv test.c -o test.S
```

### Full Options List

| Option | Required | Description |
|--------|:--------:|-------------|
| `-koopa` | Yes* | Output Koopa IR (Mutual exclusion with -riscv) |
| `-riscv` | Yes* | Output RISC-V assembly (Mutual exclusion with -koopa) |
| `-perf` | No | Enable optimizations(Not yet completed) |
| `-o <file>` | Yes | Specify the output file path |
| `<input_file>` | Yes | The source code file to compile |
| `-h, --help` | No | Show help message |

*Note: You must select a compilation target mode.*

## Project Structure

- `src/frontend`: Lexer (`sysy.lx`) and Parser (`sysy.y`)
- `src/ir`: AST and IR generation logic
- `src/backend`: RISC-V code generation
- `src/main.cpp`: Entry point with C++20 module imports
- `include`: Header files, including Koopa wrapper and logging

## Todo List

- [ ] Complete implementation notes and documentation
- [ ] Optimizations for IR and RISC-V
- [ ] Floating point support (maybe)

