#include <iostream>
#include <vector>
#include <string>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/ExecutionEngine/MCJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

// === Runtime Print Function Implementations ===
extern "C" {

    void print_int(int x) {
        std::cout << "[print] int: " << x << std::endl;
    }

    void print_float(float f) {
        std::cout << "[print] float: " << f << std::endl;
    }

    void print_char(char c) {
        std::cout << "[print] char: " << c << std::endl;
    }

    void print_str(const char* s) {
        std::cout << "[print] string: " << s << std::endl;
    }

}

// === Main JIT Runner ===
int main(int argc, char** argv) {
    // Initialize LLVM JIT
    InitializeNativeTarget();
    InitializeNativeTargetAsmPrinter();
    InitializeNativeTargetAsmParser();

    LLVMContext Context;
    SMDiagnostic Err;

    // Load the LLVM IR from file
    std::unique_ptr<Module> Mod = parseIRFile("out.ll", Err, Context);
    if (!Mod) {
        Err.print("jit_runner", errs());
        std::cerr << "Likely a malformed or outdated output.ll — did you overwrite it correctly?" << std::endl;
        return 1;
    }

    Module* M = Mod.get();  // Save pointer to module

    // Create the execution engine
    std::string errorStr;
    ExecutionEngine* EE = EngineBuilder(std::move(Mod))
        .setErrorStr(&errorStr)
        .setEngineKind(EngineKind::JIT)
        .create();

    if (!EE) {
        std::cerr << "Failed to create ExecutionEngine: " << errorStr << std::endl;
        return 1;
    }

    // Bind external functions
    auto map = [&](const char* name, void* addr) {
        if (Function* fn = M->getFunction(name)) {
            EE->addGlobalMapping(fn, addr);
        }
    };

    map("print",        (void*)&print_int);
    map("print_int",    (void*)&print_int);
    map("print_float",  (void*)&print_float);
    map("print_char",   (void*)&print_char);
    map("print_str",    (void*)&print_str);
    map("print_string", (void*)&print_str); // ✅ FIXED: binds @print_string in IR

    // Finalize and run
    EE->finalizeObject();

    Function* MainFunc = EE->FindFunctionNamed("main");
    if (!MainFunc) {
        std::cerr << "Function 'main' not found!" << std::endl;
        return 1;
    }

    std::vector<GenericValue> noArgs;
    GenericValue result = EE->runFunction(MainFunc, noArgs);

    std::cout << "[main returned]: " << result.IntVal.getSExtValue() << std::endl;
    return 0;
}
