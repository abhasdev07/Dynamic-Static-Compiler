#ifndef CODEGEN_LLVM_H
#define CODEGEN_LLVM_H

#include <stdio.h>
#include "ir.h"

void codegen_llvm_emit(IRList *ir, FILE *out);

#endif
