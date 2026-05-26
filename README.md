
# Dynamic Static Compiler

## Lightweight Multi-Stage Educational Compiler with LLVM IR Backend

---

# Overview

Dynamic Static Compiler is a lightweight educational compiler written in C that demonstrates the complete compilation pipeline of a modern compiler system.

The project implements:

- Lexical Analysis
- Syntax Analysis
- Abstract Syntax Tree (AST) Generation
- Semantic Analysis
- Intermediate Representation (IR) Generation
- IR Optimization
- LLVM IR Generation
- Runtime Interpretation

The compiler uses a custom C-like language and supports:

- integers
- characters
- strings
- arrays
- functions
- loops
- conditional statements
- semantic validation
- LLVM backend generation

---

# Compiler Pipeline

```text
Source Code
     ↓
Lexer
     ↓
Parser
     ↓
AST Generation
     ↓
Semantic Analyzer
     ↓
IR Generator
     ↓
Optimizer
     ↓
LLVM IR Generator
     ↓
Runtime Interpreter
```

---

# Supported Features

| Feature | Support |
|---|---|
| int datatype | ✅ |
| char datatype | ✅ |
| string datatype | ✅ |
| arrays | ✅ |
| if-else | ✅ |
| while loops | ✅ |
| for loops | ✅ |
| functions | ✅ |
| semantic analysis | ✅ |
| LLVM backend | ✅ |
| IR optimization | ✅ |
| runtime execution | ✅ |
| string indexing | ✅ |
| multi-argument print | ✅ |

---

# Supported Datatypes

| Datatype | Support |
|---|---|
| int | ✅ FULL |
| char | ✅ FULL |
| string | ✅ STABLE |
| float | ⚠️ Partial Parser/IR Support |
| void | ❌ Unsupported |

---

# Language Syntax

## Variable Declaration

```c
int x = 10;
char ch = 'A';
string msg = "Hello";
```

---

## Arithmetic Operations

```c
x = x + 1;
y = x * 2;
```

Supported operators:

```text
+
-
*
/
%
```

---

## Conditional Statements

```c
if (x > 10) {

    print(111);

} else {

    print(222);
}
```

---

## While Loops

```c
while (i < 5) {

    print(i);

    i++;
}
```

---

## For Loops

```c
for (int i = 0; i < 5; i++) {

    print(i);
}
```

---

## Arrays

```c
int arr[5];

arr[0] = 10;

print(arr[0]);
```

---

## String Indexing

```c
string msg = "Compiler";

print(msg[0]);
```

---

## Functions

```c
int add(int a, int b) {

    return a + b;
}
```

---

# Semantic Analysis

## Supported Semantic Checks

| Semantic Check | Support |
|---|---|
| Undeclared variable detection | ✅ |
| Redeclared variable detection | ✅ |
| Function existence checking | ✅ |
| Function argument count checking | ✅ |
| Assignment type validation | ✅ |

---

# Intermediate Representation (IR)

Example IR:

```text
t1 = load y
t2 = 1
t3 = t1 + t2
store x, t3
```

---

# LLVM Backend

Example LLVM IR:

```llvm
define i32 @add(i32 %a, i32 %b) {
entry:
  %v1 = add i32 %a, %b
  ret i32 %v1
}
```

---

# Runtime Execution

The runtime interpreter supports:

- arithmetic execution
- loops
- functions
- arrays
- branching
- strings
- chars

---

# Project File Structure

| File | Purpose |
|---|---|
| main.c | Compiler pipeline driver |
| lexer.c / lexer.h | Lexical analysis |
| parser.c / parser.h | Syntax analysis |
| semantic.c / semantic.h | Semantic analysis |
| ir.c / ir.h | Intermediate Representation |
| optimizer.c / optimizer.h | Optimization |
| codegen_llvm.c / codegen_llvm.h | LLVM IR generation |
| ir_interp.c / ir_interp.h | Runtime interpreter |
| build.bat | Build script |

---

# Build Instructions

## Build

```bash
build.bat
```

## Run

```bash
compiler.exe input.txt
```

---

# Example Program

```c
int add(int a, int b) {

    return a + b;
}

int main() {

    string msg = "Compiler";

    char ch = 'A';

    int arr[3];

    arr[0] = 5;
    arr[1] = 10;

    arr[2] = add(arr[0], arr[1]);

    print(msg);

    print(ch);

    print(arr[2]);

    return 0;
}
```

---

# Current Limitations

| Feature | Status |
|---|---|
| Full float runtime | ❌ |
| Float printing | ❌ |
| String-return functions | ❌ |
| Void functions | ❌ |
| Dynamic memory | ❌ |
| Pointers | ❌ |
| Structs | ❌ |
| Classes/OOP | ❌ |

---

# Future Improvements

Possible future enhancements:

- stable float runtime support
- generalized type system
- recursion
- pointer support
- struct support
- advanced optimization passes
- assembly generation
- register allocation

---

# Resume Description

## Dynamic Static Compiler

- Developed a lightweight educational compiler implementing lexical analysis, parsing, semantic analysis, IR generation, optimization, LLVM IR generation, and runtime execution.
- Designed a custom C-like language supporting functions, arrays, loops, strings, characters, and semantic validation.
- Built a modular compiler pipeline with a custom intermediate representation and LLVM backend generation.

---

# Conclusion

Dynamic Static Compiler demonstrates the architecture of a modern educational compiler pipeline while remaining lightweight, modular, and educationally focused.

The project successfully implements:

- lexical analysis
- syntax analysis
- semantic analysis
- IR generation
- optimization
- LLVM IR generation
- runtime execution

making it a strong systems programming and compiler-design project.
