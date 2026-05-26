#include <stdio.h>
#include "optimizer.h"

void ir_optimize(IRList *list) {
    IRInst *curr = list->head;

    printf("[Optimizer] Starting optimization pass...\n");

    while (curr != NULL) {
        // Skip control flow instructions
        if (curr->op == IR_LABEL || curr->op == IR_JUMP || curr->op == IR_JUMP_IF_FALSE || curr->op == IR_RETURN) {
            curr = curr->next;
            continue;
        }

        // Constant folding for binary ops
        if (curr->op == IR_ADD || curr->op == IR_SUB || curr->op == IR_MUL || curr->op == IR_DIV ||
            curr->op == IR_MOD || curr->op == IR_EQ || curr->op == IR_NEQ || curr->op == IR_LT ||
            curr->op == IR_GT || curr->op == IR_LE || curr->op == IR_GE || curr->op == IR_AND || curr->op == IR_OR ||
            curr->op == IR_BIT_AND || curr->op == IR_BIT_OR || curr->op == IR_BIT_XOR || curr->op == IR_SHL || curr->op == IR_SHR) {

            IRInst *src1 = NULL, *src2 = NULL;
            IRInst *scan = list->head;

            // ✅ Stop as soon as both are found
            while (scan != NULL && (!src1 || !src2)) {
                if (!src1 && scan->dest == curr->src1) src1 = scan;
                if (!src2 && scan->dest == curr->src2) src2 = scan;
                scan = scan->next;
            }

            if (src1 && src2 && src1->op == IR_LOAD_CONST && src2->op == IR_LOAD_CONST) {
                int result = 0;
                switch (curr->op) {
                    case IR_ADD: result = src1->value + src2->value; break;
                    case IR_SUB: result = src1->value - src2->value; break;
                    case IR_MUL: result = src1->value * src2->value; break;
                    case IR_DIV: result = src1->value / src2->value; break;
                    case IR_MOD: result = src1->value % src2->value; break;
                    case IR_EQ:  result = (src1->value == src2->value); break;
                    case IR_NEQ: result = (src1->value != src2->value); break;
                    case IR_LT:  result = (src1->value < src2->value); break;
                    case IR_GT:  result = (src1->value > src2->value); break;
                    case IR_LE:  result = (src1->value <= src2->value); break;
                    case IR_GE:  result = (src1->value >= src2->value); break;
                    case IR_AND: result = (src1->value && src2->value); break;
                    case IR_OR:  result = (src1->value || src2->value); break;

                    // ✅ Bitwise ops
                    case IR_BIT_AND: result = src1->value & src2->value; break;
                    case IR_BIT_OR:  result = src1->value | src2->value; break;
                    case IR_BIT_XOR: result = src1->value ^ src2->value; break;
                    case IR_SHL:     result = src1->value << src2->value; break;
                    case IR_SHR:     result = src1->value >> src2->value; break;

                    default: continue;
                }

                printf("[Optimizer] Folding: t%d = %d (was t%d op t%d)\n",
                       curr->dest, result, curr->src1, curr->src2);

                curr->op = IR_LOAD_CONST;
                curr->value = result;
                curr->src1 = -1;
                curr->src2 = -1;
            }
        }

        
        if (curr->op == IR_FADD || curr->op == IR_FSUB || curr->op == IR_FMUL || curr->op == IR_FDIV) {
            IRInst *src1 = NULL, *src2 = NULL;
            IRInst *scan = list->head;

            while (scan != NULL && (!src1 || !src2)) {
                if (!src1 && scan->dest == curr->src1) src1 = scan;
                if (!src2 && scan->dest == curr->src2) src2 = scan;
                scan = scan->next;
            }

            if (src1 && src2 && src1->op == IR_FLOAD_CONST && src2->op == IR_FLOAD_CONST) {
                float val1 = *(float *)&src1->value;
                float val2 = *(float *)&src2->value;
                float result = 0.0;

                switch (curr->op) {
                    case IR_FADD: result = val1 + val2; break;
                    case IR_FSUB: result = val1 - val2; break;
                    case IR_FMUL: result = val1 * val2; break;
                    case IR_FDIV: result = val1 / val2; break;
                    default: break;
                }

                printf("[Optimizer] Folding: t%d = %f (was t%d op t%d)\n",
                    curr->dest, result, curr->src1, curr->src2);

                curr->op = IR_FLOAD_CONST;
                *(float *)&curr->value = result;
                curr->src1 = -1;
                curr->src2 = -1;
            }
        }

        curr = curr->next;
    }

    printf("[Optimizer] Optimization complete.\n");
}
