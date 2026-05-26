#ifndef IR_INTERP_H
#define IR_INTERP_H

#include "ir.h"

/* Execute IR starting at @main; prints program output to stdout. */
void ir_interp_run(IRList *list);

#endif
