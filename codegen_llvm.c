#include "codegen_llvm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARS 256

typedef struct {
    char name[64];
    int is_array;
    int is_string;
    int array_size;
    int emitted;
} VarInfo;

typedef struct {
    char text[256];
    int id;
} StrLit;

static VarInfo vars[MAX_VARS];
static int var_count;
static StrLit str_lits[128];
static int str_lit_count;
static int pending_args[32];
static int pending_arg_count;
static int cmp_id;

static int is_function_label(const char *label) {
    return label && strchr(label, '_') == NULL;
}

static VarInfo *find_var(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(vars[i].name, name) == 0)
            return &vars[i];
    return NULL;
}

static VarInfo *register_var(const char *name, int is_array, int is_string, int array_size) {
    VarInfo *v = find_var(name);
    if (!v) {
        if (var_count >= MAX_VARS) return NULL;
        v = &vars[var_count++];
        strncpy(v->name, name, 63);
        v->name[63] = '\0';
        v->is_array = is_array;
        v->is_string = is_string;
        v->array_size = array_size > 0 ? array_size : 100;
        v->emitted = 0;
    } else {
        if (is_array) { v->is_array = 1; if (array_size > 0) v->array_size = array_size; }
        if (is_string) v->is_string = 1;
    }
    return v;
}

static int get_or_emit_string_lit(FILE *out, const char *text) {
    for (int i = 0; i < str_lit_count; i++) {
        if (strcmp(str_lits[i].text, text) == 0)
            return str_lits[i].id;
    }
    int id = str_lit_count;
    strncpy(str_lits[str_lit_count].text, text, 255);
    str_lits[str_lit_count].id = id;
    str_lit_count++;

    int len = (int)strlen(text);
    fprintf(out, "@.strlit%d = private unnamed_addr constant [%d x i8] c\"", id, len + 1);
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c == '\\') fprintf(out, "\\5C");
        else if (c == '\n') fprintf(out, "\\0A");
        else if (c == '"') fprintf(out, "\\22");
        else fprintf(out, "%c", c);
    }
    fprintf(out, "\\00\", align 1\n");
    return id;
}

static void scan_symbols_range(IRInst *start, IRInst *end) {
    for (IRInst *inst = start; inst && inst != end; inst = inst->next) {
        if (inst->op == IR_ARRAY_DECL)
            register_var(inst->var_name, 1, 0, inst->value);
        if (inst->op == IR_STRING_DECL || inst->op == IR_STRING_INIT)
            register_var(inst->var_name, 0, 1, 0);
        if (inst->op == IR_PARAM_STORE || inst->op == IR_STORE_VAR || inst->op == IR_LOAD_VAR)
            register_var(inst->var_name, 0, 0, 0);
        if (inst->op == IR_ARRAY_LOAD || inst->op == IR_ARRAY_STORE)
            register_var(inst->var_name, 1, 0, 0);
        if (inst->op == IR_CHAR_LOAD || inst->op == IR_CHAR_STORE)
            register_var(inst->var_name, 0, 1, 0);
    }
}

static void emit_header(FILE *out) {
    fprintf(out,
        "declare void @print_int(i32)\n"
        "declare void @print_char(i8)\n"
        "declare void @print_string(i8*)\n"
        "declare void @print_float(float)\n\n");
}

static void emit_var_allocas(FILE *out) {
    for (int i = 0; i < var_count; i++) {
        VarInfo *v = &vars[i];
        if (v->emitted) continue;
        v->emitted = 1;
        if (v->is_array)
            fprintf(out, "  %%%s = alloca [%d x i32], align 4\n", v->name, v->array_size);
        else if (v->is_string)
            fprintf(out, "  %%%s = alloca i8*, align 8\n", v->name);
        else
            fprintf(out, "  %%%s = alloca i32, align 4\n", v->name);
    }
}

static void emit_cmp_to_i32(FILE *out, int dest, int cmp) {
    fprintf(out, "  %%v%d = zext i1 %%cmp%d to i32\n", dest, cmp);
}

static void emit_instruction(FILE *out, IRInst *inst) {
    switch (inst->op) {
    case IR_LOAD_CONST:
        fprintf(out, "  %%v%d = add i32 0, %d\n", inst->dest, inst->value);
        break;
    case IR_FLOAD_CONST:
        fprintf(out, "  %%v%d = fadd float 0.0, 0x%08X\n", inst->dest, inst->value);
        break;
    case IR_LOAD_VAR: {
        VarInfo *v = find_var(inst->var_name);
        if (v && v->is_string)
            fprintf(out, "  %%v%d = load i8*, i8** %%%s, align 8\n", inst->dest, inst->var_name);
        else
            fprintf(out, "  %%v%d = load i32, i32* %%%s, align 4\n", inst->dest, inst->var_name);
        break;
    }
    case IR_STORE_VAR: {
        VarInfo *v = find_var(inst->var_name);
        if (!v || !v->is_string)
            fprintf(out, "  store i32 %%v%d, i32* %%%s, align 4\n", inst->src1, inst->var_name);
        break;
    }
    case IR_STRING_INIT: {
        register_var(inst->var_name, 0, 1, 0);
        int sid = -1;
        for (int i = 0; i < str_lit_count; i++) {
            if (strcmp(str_lits[i].text, inst->label) == 0) {
                sid = str_lits[i].id;
                break;
            }
        }
        if (sid < 0) sid = 0;
        int len = (int)strlen(inst->label);
        fprintf(out, "  %%gep.%s = getelementptr inbounds [%d x i8], [%d x i8]* @.strlit%d, i32 0, i32 0\n",
                inst->var_name, len + 1, len + 1, sid);
        fprintf(out, "  store i8* %%gep.%s, i8** %%%s, align 8\n", inst->var_name, inst->var_name);
        break;
    }
    case IR_ADD: fprintf(out, "  %%v%d = add i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_SUB: fprintf(out, "  %%v%d = sub i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_MUL: fprintf(out, "  %%v%d = mul i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_DIV: fprintf(out, "  %%v%d = sdiv i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_MOD: fprintf(out, "  %%v%d = srem i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_FADD: fprintf(out, "  %%v%d = fadd float %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_FSUB: fprintf(out, "  %%v%d = fsub float %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_FMUL: fprintf(out, "  %%v%d = fmul float %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_FDIV: fprintf(out, "  %%v%d = fdiv float %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_NEG: fprintf(out, "  %%v%d = sub i32 0, %%v%d\n", inst->dest, inst->src1); break;
    case IR_LOG_NOT:
        fprintf(out, "  %%cmp%d = icmp eq i32 %%v%d, 0\n", cmp_id, inst->src1);
        emit_cmp_to_i32(out, inst->dest, cmp_id++);
        break;
    case IR_BIT_NOT:
        fprintf(out, "  %%v%d = xor i32 %%v%d, -1\n", inst->dest, inst->src1);
        break;
    case IR_EQ:
        fprintf(out, "  %%cmp%d = icmp eq i32 %%v%d, %%v%d\n", cmp_id, inst->src1, inst->src2);
        emit_cmp_to_i32(out, inst->dest, cmp_id++);
        break;
    case IR_NEQ:
        fprintf(out, "  %%cmp%d = icmp ne i32 %%v%d, %%v%d\n", cmp_id, inst->src1, inst->src2);
        emit_cmp_to_i32(out, inst->dest, cmp_id++);
        break;
    case IR_LT:
        fprintf(out, "  %%cmp%d = icmp slt i32 %%v%d, %%v%d\n", cmp_id, inst->src1, inst->src2);
        emit_cmp_to_i32(out, inst->dest, cmp_id++);
        break;
    case IR_GT:
        fprintf(out, "  %%cmp%d = icmp sgt i32 %%v%d, %%v%d\n", cmp_id, inst->src1, inst->src2);
        emit_cmp_to_i32(out, inst->dest, cmp_id++);
        break;
    case IR_LE:
        fprintf(out, "  %%cmp%d = icmp sle i32 %%v%d, %%v%d\n", cmp_id, inst->src1, inst->src2);
        emit_cmp_to_i32(out, inst->dest, cmp_id++);
        break;
    case IR_GE:
        fprintf(out, "  %%cmp%d = icmp sge i32 %%v%d, %%v%d\n", cmp_id, inst->src1, inst->src2);
        emit_cmp_to_i32(out, inst->dest, cmp_id++);
        break;
    case IR_AND:
        fprintf(out, "  %%cmp%d = icmp ne i32 %%v%d, 0\n", cmp_id, inst->src1);
        fprintf(out, "  %%cmp%d = icmp ne i32 %%v%d, 0\n", cmp_id + 1, inst->src2);
        fprintf(out, "  %%cmp%d = and i1 %%cmp%d, %%cmp%d\n", cmp_id + 2, cmp_id, cmp_id + 1);
        emit_cmp_to_i32(out, inst->dest, cmp_id + 2);
        cmp_id += 3;
        break;
    case IR_OR:
        fprintf(out, "  %%cmp%d = icmp ne i32 %%v%d, 0\n", cmp_id, inst->src1);
        fprintf(out, "  %%cmp%d = icmp ne i32 %%v%d, 0\n", cmp_id + 1, inst->src2);
        fprintf(out, "  %%cmp%d = or i1 %%cmp%d, %%cmp%d\n", cmp_id + 2, cmp_id, cmp_id + 1);
        emit_cmp_to_i32(out, inst->dest, cmp_id + 2);
        cmp_id += 3;
        break;
    case IR_BIT_AND: fprintf(out, "  %%v%d = and i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_BIT_OR:  fprintf(out, "  %%v%d = or i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_BIT_XOR: fprintf(out, "  %%v%d = xor i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_SHL: fprintf(out, "  %%v%d = shl i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_SHR: fprintf(out, "  %%v%d = ashr i32 %%v%d, %%v%d\n", inst->dest, inst->src1, inst->src2); break;
    case IR_CAST_I2F: fprintf(out, "  %%v%d = sitofp i32 %%v%d to float\n", inst->dest, inst->src1); break;
    case IR_CAST_F2I: fprintf(out, "  %%v%d = fptosi float %%v%d to i32\n", inst->dest, inst->src1); break;
    case IR_MOVE: fprintf(out, "  %%v%d = add i32 %%v%d, 0\n", inst->dest, inst->src1); break;
    case IR_STRING_CONST:
        get_or_emit_string_lit(out, inst->var_name);
        fprintf(out, "  %%v%d = add i32 0, 0\n", inst->dest);
        break;
    case IR_PRINT:
        fprintf(out, "  call void @print_int(i32 %%v%d)\n", inst->src1);
        break;
    case IR_PRINT_STRING: {
        int sid = get_or_emit_string_lit(out, inst->var_name);
        int len = (int)strlen(inst->var_name);
        fprintf(out, "  %%pstr%d = getelementptr inbounds [%d x i8], [%d x i8]* @.strlit%d, i32 0, i32 0\n",
                cmp_id, len + 1, len + 1, sid);
        fprintf(out, "  call void @print_string(i8* %%pstr%d)\n", cmp_id);
        cmp_id++;
        break;
    }
    case IR_PRINT_STRING_PTR:
        fprintf(out, "  call void @print_string(i8* %%v%d)\n", inst->src1);
        break;
    case IR_PRINT_CHAR: {
        fprintf(out, "  %%pch%d = trunc i32 %%v%d to i8\n", cmp_id, inst->src1);
        fprintf(out, "  call void @print_char(i8 %%pch%d)\n", cmp_id);
        cmp_id++;
        break;
    }
    case IR_ARRAY_DECL:
        register_var(inst->var_name, 1, 0, inst->value);
        break;
    case IR_ARRAY_LOAD: {
        VarInfo *v = find_var(inst->var_name);
        int sz = v ? v->array_size : 100;
        fprintf(out, "  %%arrp%d = getelementptr inbounds [%d x i32], [%d x i32]* %%%s, i32 0, i32 %%v%d\n",
                cmp_id, sz, sz, inst->var_name, inst->src1);
        fprintf(out, "  %%v%d = load i32, i32* %%arrp%d, align 4\n", inst->dest, cmp_id);
        cmp_id++;
        break;
    }
    case IR_ARRAY_STORE: {
        VarInfo *v = find_var(inst->var_name);
        int sz = v ? v->array_size : 100;
        fprintf(out, "  %%arrp%d = getelementptr inbounds [%d x i32], [%d x i32]* %%%s, i32 0, i32 %%v%d\n",
                cmp_id, sz, sz, inst->var_name, inst->src1);
        fprintf(out, "  store i32 %%v%d, i32* %%arrp%d, align 4\n", inst->src2, cmp_id);
        cmp_id++;
        break;
    }
    case IR_CHAR_LOAD: {
        int id = cmp_id++;
        fprintf(out, "  %%strp%d = load i8*, i8** %%%s, align 8\n", id, inst->var_name);
        fprintf(out, "  %%chp%d = getelementptr i8, i8* %%strp%d, i32 %%v%d\n", id, id, inst->src1);
        fprintf(out, "  %%chb%d = load i8, i8* %%chp%d\n", id, id);
        fprintf(out, "  %%v%d = zext i8 %%chb%d to i32\n", inst->dest, id);
        break;
    }
    case IR_CHAR_STORE: {
        int id = cmp_id++;
        fprintf(out, "  %%strp%d = load i8*, i8** %%%s, align 8\n", id, inst->var_name);
        fprintf(out, "  %%chp%d = getelementptr i8, i8* %%strp%d, i32 %%v%d\n", id, id, inst->src1);
        fprintf(out, "  %%chv%d = trunc i32 %%v%d to i8\n", id, inst->src2);
        fprintf(out, "  store i8 %%chv%d, i8* %%chp%d\n", id, id);
        break;
    }
    case IR_ARG:
        if (pending_arg_count < 32)
            pending_args[pending_arg_count++] = inst->src1;
        break;
    case IR_PARAM_STORE:
        fprintf(out, "  store i32 %%p%d, i32* %%%s, align 4\n", inst->value, inst->var_name);
        break;
    case IR_CALL:
        fprintf(out, "  %%v%d = call i32 @%s(", inst->dest, inst->label);
        for (int i = 0; i < pending_arg_count; i++)
            fprintf(out, "i32 %%v%d%s", pending_args[i], (i + 1 < pending_arg_count) ? ", " : "");
        fprintf(out, ")\n");
        pending_arg_count = 0;
        break;
    case IR_RETURN:
        fprintf(out, "  ret i32 %%v%d\n", inst->src1);
        break;
    case IR_LABEL:
        if (!is_function_label(inst->label))
            fprintf(out, "%s:\n", inst->label);
        break;
    case IR_JUMP:
        fprintf(out, "  br label %%%s\n", inst->label);
        break;
    case IR_JUMP_IF_FALSE:
        fprintf(out, "  %%cmp%d = icmp eq i32 %%v%d, 0\n", cmp_id, inst->src1);
        fprintf(out, "  br i1 %%cmp%d, label %%%s, label %%ifcont%d\n", cmp_id, inst->label, cmp_id);
        fprintf(out, "ifcont%d:\n", cmp_id);
        cmp_id++;
        break;
    case IR_STRING_DECL:
        register_var(inst->var_name, 0, 1, 0);
        break;
    default:
        fprintf(out, "  ; unhandled op %d\n", inst->op);
        break;
    }
}

static int count_function_params(IRInst *start) {
    int n = 0;
    for (IRInst *i = start->next; i && i->op == IR_PARAM_STORE; i = i->next)
        n++;
    return n;
}

static void emit_function(FILE *out, IRInst **cursor) {
    IRInst *inst = *cursor;
    if (!inst || inst->op != IR_LABEL || !is_function_label(inst->label))
        return;

    const char *name = inst->label;
    int nparams = count_function_params(inst);

    IRInst *body = inst->next;
    IRInst *end = NULL;
    for (IRInst *s = body; s; s = s->next) {
        if (s->op == IR_LABEL && is_function_label(s->label)) {
            end = s;
            break;
        }
    }

    var_count = 0;
    scan_symbols_range(body, end);

    fprintf(out, "define i32 @%s(", name);
    for (int i = 0; i < nparams; i++)
        fprintf(out, "i32 %%p%d%s", i, (i + 1 < nparams) ? ", " : "");
    fprintf(out, ") {\nentry:\n");

    for (int i = 0; i < var_count; i++)
        vars[i].emitted = 0;
    emit_var_allocas(out);

    pending_arg_count = 0;
    cmp_id = 0;
    int returned = 0;

    for (inst = body; inst != end; inst = inst->next) {
        if (inst->op == IR_RETURN)
            returned = 1;
        emit_instruction(out, inst);
    }
    *cursor = end;
    if (!returned)
        fprintf(out, "  ret i32 0\n");
    fprintf(out, "}\n\n");
}

static void precollect_literals(IRList *ir, FILE *out) {
    for (IRInst *inst = ir->head; inst; inst = inst->next) {
        if (inst->op == IR_STRING_INIT || inst->op == IR_PRINT_STRING)
            get_or_emit_string_lit(out, inst->label ? inst->label : inst->var_name);
        if (inst->op == IR_STRING_CONST && inst->var_name)
            get_or_emit_string_lit(out, inst->var_name);
    }
}

void codegen_llvm_emit(IRList *ir, FILE *out) {
    str_lit_count = 0;
    cmp_id = 0;

    emit_header(out);
    precollect_literals(ir, out);
    fprintf(out, "\n");

    IRInst *cur = ir->head;
    while (cur) {
        if (cur->op == IR_LABEL && is_function_label(cur->label)) {
            emit_function(out, &cur);
            if (!cur) break;
        } else {
            cur = cur->next;
        }
    }
}
