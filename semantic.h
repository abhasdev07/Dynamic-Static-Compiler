#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

/* Returns 0 on success; exits on semantic error. */
int semantic_check_program(ASTList *program);

#endif
