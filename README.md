
# 🔧 The Lightweight Meta Assembly & JIT Compiler

A lightweight compiler and JIT executor for a simplified C-like language. This project features full compilation pipeline support, including lexing, parsing, AST generation, IR construction, optimization, and LLVM-based JIT execution.

---

## 🧠 Project Overview

This compiler was built as part of a curriculum project for **B.Tech CSE - Compiler Design (CD-VI-T100)**. It aims to demonstrate key principles of compiler construction through a fully working implementation using C and LLVM.

### ✅ Features

- Lexical Analysis (tokenization of input)
- Recursive-Descent Parser and AST generation
- Intermediate Representation (three-address code)
- Constant folding and basic optimizations
- LLVM JIT Execution using MCJIT
- Support for:
  - `int`, `char`, `float`, and `string` types
  - Arithmetic, bitwise, logical, and ternary operations
  - Type casting: `int ↔ float`
  - Arrays and function calls
  - Control flow: `if`, `else`, `while`, `for`
  - Built-in `print()` functionality

---

## 🗂️ Directory Structure

```
.
├── main.c               # Entry point: invokes parsing, IR generation, and codegen
├── lexer.c / lexer.h    # Lexical analyzer
├── parser.c / parser.h  # Syntax analyzer and AST construction
├── ir.c / ir.h          # IR generation logic
├── optimizer.c          # Constant folding and IR simplification
├── codegen_llvm.c       # LLVM IR generation from custom IR
├── jit_runner.cpp       # Executes LLVM IR using MCJIT
├── input.txt            # Sample test input file
├── output.ll            # Generated LLVM IR
└── README.md            # Project documentation
```

---

## 🚀 Getting Started

### 🔧 Prerequisites

- GCC (for compiling C components)
- Clang/LLVM (for JIT execution)
- CMake (recommended for cross-platform builds)

### 🔨 Build & Run (Manual)

1. **Compile Compiler Frontend (C modules)**

```bash
gcc -o compiler main.c lexer.c parser.c ir.c optimizer.c codegen_llvm.c -lm
```

2. **Generate LLVM IR**

```bash
./compiler input.txt
```

This will produce `output.ll`.

3. **Run JIT Execution**

```bash
g++ -o jit jit_runner.cpp `llvm-config --cxxflags --ldflags --system-libs --libs all`
./jit
```

---

## 📄 Sample Input Format (`input.txt`)

```c
int add(int a, int b) {
    return a + b;
}

int main() {
    int x = 3;
    int y = 4;
    int z = add(x, y);
    print(z);
    return z;
}
```

Supports:
- Function calls
- Arithmetic operations
- Conditionals and loops
- Type casting and strings

---

## ✅ Testing Status

| Module         | Status |
|----------------|--------|
| Lexer          | ✅ Pass |
| Parser & AST   | ✅ Pass |
| IR Generation  | ✅ Pass |
| Optimizer      | ✅ Pass |
| JIT Execution  | ✅ Pass |
| Function Calls | ✅ Pass |
| Control Flow   | ✅ Pass |
| Error Recovery | ✅ Basic |
| Type Support   | ✅ Int, Float, Char, String |

---

## 🔮 Future Enhancements

- Interactive debugger with breakpoints
- Advanced optimization passes (e.g. dead code elimination)
- Multi-file compilation
- Graphical UI for AST/IR visualization
- Dynamic memory model and garbage collection

---

## 👥 Authors

- **Saloni Gupta** – 22022122 *(Team Lead)*
- **Harleen Kaur** – 22022609
- **Srijan Chauhan** – 22022739

📧 For queries, reach out to [Flow Fixers](mailto:charu2004gupta@gmail.com)

---

## 🛠 License

This project is developed for academic purposes. Feel free to fork and build upon it with attribution.
