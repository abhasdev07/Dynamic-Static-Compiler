@echo off
gcc -o compiler.exe main.c parser.c lexer.c ir.c optimizer.c codegen_llvm.c ir_interp.c -Wall
if %errorlevel% neq 0 (
    echo Build failed.
) else (
    echo Build succeeded.
    echo Running compiler on input.txt...
    .\compiler.exe input.txt
    echo.
    echo Done. LLVM IR is in output.ll and out.ll
)