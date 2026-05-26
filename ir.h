
#ifndef IR_H
#define IR_H

typedef struct ASTNode ASTNode;
typedef struct ASTList ASTList;

typedef enum {
    IR_FLOAD_CONST,
    IR_FADD,
    IR_FSUB,
    IR_FMUL,
    IR_FDIV,
    IR_LOAD_CONST,
    IR_LOAD_VAR,
    IR_STORE_VAR,
    IR_ADD,
    IR_SUB,
    IR_MUL,
    IR_DIV,
    IR_MOD, 
    IR_NEG,
    IR_STRING_CONST,
    IR_PRINT_STRING,
    IR_PRINT_STRING_PTR,
    IR_PRINT_CHAR,
    IR_CAST_I2F,
    IR_CAST_F2I,
    IR_LOG_NOT,
    IR_BIT_NOT,
    IR_EQ,
    IR_NEQ,
    IR_LT,
    IR_GT,
    IR_LE,
    IR_GE,
    IR_AND,
    IR_OR,
    IR_BIT_AND,
    IR_BIT_OR,
    IR_BIT_XOR,
    IR_SHL,
    IR_SHR,
    IR_PRINT,
    IR_LABEL,
    IR_JUMP,
    IR_JUMP_IF_FALSE,
    IR_ARRAY_DECL,
    IR_ARRAY_LOAD,
    IR_ARRAY_STORE,
    IR_STRING_DECL,
    IR_STRING_INIT,
    IR_CHAR_LOAD,
    IR_CHAR_STORE,
    IR_CALL,
    IR_MOVE,  
    IR_ARG,
    IR_ARG_LOAD,
    IR_PARAM_STORE,
    IR_RETURN
} IROp;

typedef struct IRInst {
    IROp op;
    int dest;
    int src1;
    int src2;
    int value;
    char *var_name;
    char *label;
    struct IRInst *next;
} IRInst;

typedef struct {
    IRInst *head;
    IRInst *tail;
    IRInst *globals_head;
    IRInst *globals_tail;
    int temp_count;
    int label_count;
} IRList;

void ir_list_init(IRList *list);
int ir_emit_const(IRList *list, int value);
int ir_emit_binop(IRList *list, IROp op, int left, int right);
int ir_emit_assign(IRList *list, const char *var_name, int src);
int ir_emit_load_var(IRList *list, const char *var_name);
void ir_emit_label(IRList *list, const char *label);
void ir_emit_jump(IRList *list, const char *label);
void ir_emit_jump_if_false(IRList *list, int cond, const char *label);
void ir_emit_return(IRList *list, int value);
int ir_emit_fconst(IRList *list, float value);
int ir_generate_expr(IRList *list, ASTNode *node);
void ir_generate(IRList *list, ASTNode *node);
void ir_generate_program(IRList *list, ASTList *program);
int ir_emit_string(IRList *list, const char *str);
int ir_emit_array_store(IRList *list, const char *array_name, int index, int value);
int ir_emit_array_load(IRList *list, const char *array_name, int index);
int ir_emit_cast(IRList *list, IROp op, int src);
int ir_emit_assign_temp(IRList *list, int dest, int src);
void ir_print(IRList *list);
void ir_free(IRList *list);

#endif
