#include "codegen_llvm.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VARS 256
#define MAX_STRLITS 128
#define MAX_FUNCS 64

typedef struct {
    char name[64];
    int is_array;
    int is_string;
    int is_char;
    int is_float;
    int array_size;
    int emitted;
} VarInfo;

typedef struct {
    char text[256];
    int id;
    int len;
    int emitted;
} StrLit;

static VarInfo vars[MAX_VARS];
static int var_count;
static StrLit str_lits[MAX_STRLITS];
static int str_lit_count;
static int cmp_id;

typedef struct {
    char name[64];
    int ret_char;
    int nparams;
    int param_char[16];
} FuncSig;

static FuncSig func_sigs[MAX_FUNCS];
static int func_sig_count;
static int current_func_ret_char;
static int pending_args[16];
static int pending_arg_count;

static int is_function_label(const char *label) {
    return label && strchr(label, '_') == NULL;
}

static FuncSig *lookup_func_sig(const char *name) {
    for (int i = 0; i < func_sig_count; i++)
        if (strcmp(func_sigs[i].name, name) == 0)
            return &func_sigs[i];
    return NULL;
}

static void register_func_sig(IRInst *label_inst, IRInst *body, IRInst *end) {
    if (func_sig_count >= MAX_FUNCS)
        return;
    FuncSig *f = &func_sigs[func_sig_count++];
    strncpy(f->name, label_inst->label, 63);
    f->name[63] = '\0';
    f->ret_char = (label_inst->value == TOKEN_CHAR);
    f->nparams = 0;
    for (IRInst *i = body; i && i != end; i = i->next) {
        if (i->op == IR_PARAM_STORE) {
            if (f->nparams < 16)
                f->param_char[f->nparams++] = (i->src2 == TOKEN_CHAR);
        } else if (i->op == IR_CHAR_DECL ||
                   i->op == IR_STRING_DECL ||
                   i->op == IR_ARRAY_DECL) {
            continue;
        } else {
            break;
        }
    }
}

static void scan_all_function_sigs(IRList *ir) {
    func_sig_count = 0;
    for (IRInst *inst = ir->head; inst; inst = inst->next) {
        if (inst->op != IR_LABEL || !is_function_label(inst->label))
            continue;
        IRInst *body = inst->next;
        IRInst *end = NULL;
        for (IRInst *s = body; s; s = s->next) {
            if (s->op == IR_LABEL && is_function_label(s->label)) {
                end = s;
                break;
            }
        }
        register_func_sig(inst, body, end);
    }
}

static VarInfo *find_var(const char *name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(vars[i].name, name) == 0)
            return &vars[i];
    return NULL;
}

static VarInfo *register_var(const char *name,
                             int is_array,
                             int is_string,
                             int is_char,
                             int is_float,
                             int array_size) {
    VarInfo *v = find_var(name);
    if (!v) {
        if (var_count >= MAX_VARS)
            return NULL;
        v = &vars[var_count++];
        strncpy(v->name, name, 63);
        v->name[63] = '\0';
        v->is_array = is_array;
        v->is_string = is_string;
        v->is_char = is_char;
        v->is_float = is_float;
        v->array_size = array_size > 0 ? array_size : 100;
        v->emitted = 0;
    } else {
        if (is_array) {
            v->is_array = 1;
            if (array_size > 0)
                v->array_size = array_size;
        }
        if (is_string)
            v->is_string = 1;
        if (is_char)
            v->is_char = 1;
        if (is_float)
            v->is_float = 1;
    }
    return v;
}

static int strlit_id(const char *text) {
    for (int i = 0; i < str_lit_count; i++) {
        if (strcmp(str_lits[i].text, text) == 0)
            return str_lits[i].id;
    }
    if (str_lit_count >= MAX_STRLITS)
        return 0;
    StrLit *s = &str_lits[str_lit_count++];
    strncpy(s->text, text, 255);
    s->text[255] = '\0';
    s->len = (int)strlen(text) + 1;
    s->id = str_lit_count - 1;
    s->emitted = 0;
    return s->id;
}

static void emit_string_globals(FILE *out) {
    for (int i = 0; i < str_lit_count; i++) {
        if (str_lits[i].emitted)
            continue;
        str_lits[i].emitted = 1;
        fprintf(out,
            "@.str.%d = private unnamed_addr constant [%d x i8] c\"",
            str_lits[i].id,
            str_lits[i].len);
        for (const char *p = str_lits[i].text; *p; p++) {
            if (*p == '\\' || *p == '"')
                fputc('\\', out);
            fputc(*p, out);
        }
        fprintf(out, "\\00\", align 1\n");
    }
}

static void scan_symbols_range(IRInst *start, IRInst *end) {
    for (IRInst *inst = start; inst && inst != end; inst = inst->next) {
        if (inst->op == IR_ARRAY_DECL)
            register_var(inst->var_name, 1, 0, 0, 0, inst->value);
        if (inst->op == IR_STRING_DECL || inst->op == IR_STRING_INIT)
            register_var(inst->var_name, 0, 1, 0, 0, 0);
        if (inst->op == IR_CHAR_DECL)
            register_var(inst->var_name, 0, 0, 1, 0, 0);
        if (inst->op == IR_PARAM_STORE)
            register_var(inst->var_name, 0, 0,
                         inst->src2 == TOKEN_CHAR, 0, 0);
        if (inst->op == IR_STORE_VAR || inst->op == IR_LOAD_VAR)
            register_var(inst->var_name, 0, 0, 0, 0, 0);
        if (inst->op == IR_ARRAY_LOAD || inst->op == IR_ARRAY_STORE ||
            inst->op == IR_CHAR_LOAD || inst->op == IR_CHAR_STORE)
            register_var(inst->var_name, 0, 0, 0, 0, 0);
        if (inst->op == IR_STRING_INIT || inst->op == IR_PRINT_STRING)
            strlit_id(inst->label ? inst->label :
                      (inst->var_name ? inst->var_name : ""));
        if (inst->op == IR_PRINT_STRING && inst->var_name)
            strlit_id(inst->var_name);
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
        if (v->emitted)
            continue;
        v->emitted = 1;
        if (v->is_array)
            fprintf(out, "  %%%s = alloca [%d x i32], align 4\n",
                    v->name, v->array_size);
        else if (v->is_string)
            fprintf(out, "  %%%s = alloca i8*, align 8\n", v->name);
        else if (v->is_char)
            fprintf(out, "  %%%s = alloca i8, align 1\n", v->name);
        else if (v->is_float)
            fprintf(out, "  %%%s = alloca float, align 4\n", v->name);
        else
            fprintf(out, "  %%%s = alloca i32, align 4\n", v->name);
    }
}

static void emit_string_ptr(FILE *out, int sid, const char *reg) {
    fprintf(out,
        "  %s = getelementptr inbounds [%d x i8], [%d x i8]* @.str.%d, i64 0, i64 0\n",
        reg, str_lits[sid].len, str_lits[sid].len, sid);
}

static void emit_instruction(FILE *out, IRInst *inst) {
    switch (inst->op) {

    case IR_LOAD_CONST:
        fprintf(out, "  %%v%d = add i32 0, %d\n", inst->dest, inst->value);
        break;

    case IR_FLOAD_CONST:
        fprintf(out, "  %%v%d = fadd float 0.0, 0x%08X\n",
                inst->dest, inst->value);
        break;

    case IR_LOAD_VAR: {
        VarInfo *v = find_var(inst->var_name);
        if (v && v->is_char) {
            fprintf(out,
                "  %%c%d = load i8, i8* %%%s, align 1\n",
                inst->dest, inst->var_name);
            fprintf(out,
                "  %%v%d = zext i8 %%c%d to i32\n",
                inst->dest, inst->dest);
        } else if (v && v->is_float) {
            fprintf(out,
                "  %%v%d = load float, float* %%%s, align 4\n",
                inst->dest, inst->var_name);
        } else if (v && v->is_string) {
            fprintf(out,
                "  %%v%d = load i8*, i8** %%%s, align 8\n",
                inst->dest, inst->var_name);
        } else {
            fprintf(out,
                "  %%v%d = load i32, i32* %%%s, align 4\n",
                inst->dest, inst->var_name);
        }
        break;
    }

    case IR_STORE_VAR: {
        VarInfo *v = find_var(inst->var_name);
        if (v && v->is_char) {
            fprintf(out,
                "  %%sc%d = trunc i32 %%v%d to i8\n",
                cmp_id, inst->src1);
            fprintf(out,
                "  store i8 %%sc%d, i8* %%%s, align 1\n",
                cmp_id, inst->var_name);
            cmp_id++;
        } else if (v && v->is_float) {
            fprintf(out,
                "  store float %%v%d, float* %%%s, align 4\n",
                inst->src1, inst->var_name);
        } else if (v && v->is_string) {
            fprintf(out,
                "  store i8* %%v%d, i8** %%%s, align 8\n",
                inst->src1, inst->var_name);
        } else {
            fprintf(out,
                "  store i32 %%v%d, i32* %%%s, align 4\n",
                inst->src1, inst->var_name);
        }
        break;
    }

    case IR_STRING_INIT: {
        int sid = strlit_id(inst->label);
        char reg[32];
        snprintf(reg, sizeof(reg), "%%strinit%d", cmp_id++);
        emit_string_ptr(out, sid, reg);
        fprintf(out, "  store i8* %s, i8** %%%s, align 8\n",
                reg, inst->var_name);
        break;
    }

    case IR_ADD:
        fprintf(out, "  %%v%d = add i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_SUB:
        fprintf(out, "  %%v%d = sub i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_MUL:
        fprintf(out, "  %%v%d = mul i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_DIV:
        fprintf(out, "  %%v%d = sdiv i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_MOD:
        fprintf(out, "  %%v%d = srem i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_NEG:
        fprintf(out, "  %%v%d = sub i32 0, %%v%d\n",
                inst->dest, inst->src1);
        break;
    case IR_LOG_NOT:
        fprintf(out,
            "  %%cmp%d = icmp eq i32 %%v%d, 0\n"
            "  %%v%d = zext i1 %%cmp%d to i32\n",
            cmp_id, inst->src1,
            inst->dest, cmp_id);
        cmp_id++;
        break;
    case IR_BIT_NOT:
        fprintf(out, "  %%v%d = xor i32 %%v%d, -1\n",
                inst->dest, inst->src1);
        break;
    case IR_EQ:
        fprintf(out,
            "  %%cmp%d = icmp eq i32 %%v%d, %%v%d\n"
            "  %%v%d = zext i1 %%cmp%d to i32\n",
            cmp_id, inst->src1, inst->src2,
            inst->dest, cmp_id);
        cmp_id++;
        break;
    case IR_NEQ:
        fprintf(out,
            "  %%cmp%d = icmp ne i32 %%v%d, %%v%d\n"
            "  %%v%d = zext i1 %%cmp%d to i32\n",
            cmp_id, inst->src1, inst->src2,
            inst->dest, cmp_id);
        cmp_id++;
        break;
    case IR_LT:
        fprintf(out,
            "  %%cmp%d = icmp slt i32 %%v%d, %%v%d\n"
            "  %%v%d = zext i1 %%cmp%d to i32\n",
            cmp_id, inst->src1, inst->src2,
            inst->dest, cmp_id);
        cmp_id++;
        break;
    case IR_GT:
        fprintf(out,
            "  %%cmp%d = icmp sgt i32 %%v%d, %%v%d\n"
            "  %%v%d = zext i1 %%cmp%d to i32\n",
            cmp_id, inst->src1, inst->src2,
            inst->dest, cmp_id);
        cmp_id++;
        break;
    case IR_LE:
        fprintf(out,
            "  %%cmp%d = icmp sle i32 %%v%d, %%v%d\n"
            "  %%v%d = zext i1 %%cmp%d to i32\n",
            cmp_id, inst->src1, inst->src2,
            inst->dest, cmp_id);
        cmp_id++;
        break;
    case IR_GE:
        fprintf(out,
            "  %%cmp%d = icmp sge i32 %%v%d, %%v%d\n"
            "  %%v%d = zext i1 %%cmp%d to i32\n",
            cmp_id, inst->src1, inst->src2,
            inst->dest, cmp_id);
        cmp_id++;
        break;
    case IR_AND:
        fprintf(out,
            "  %%cmp%d = icmp ne i32 %%v%d, 0\n"
            "  %%cmp%d = icmp ne i32 %%v%d, 0\n"
            "  %%cmp%d = and i1 %%cmp%d, %%cmp%d\n"
            "  %%v%d = zext i1 %%cmp%d to i32\n",
            cmp_id, inst->src1,
            cmp_id + 1, inst->src2,
            cmp_id + 2, cmp_id, cmp_id + 1,
            inst->dest, cmp_id + 2);
        cmp_id += 3;
        break;
    case IR_OR:
        fprintf(out,
            "  %%cmp%d = icmp ne i32 %%v%d, 0\n"
            "  %%cmp%d = icmp ne i32 %%v%d, 0\n"
            "  %%cmp%d = or i1 %%cmp%d, %%cmp%d\n"
            "  %%v%d = zext i1 %%cmp%d to i32\n",
            cmp_id, inst->src1,
            cmp_id + 1, inst->src2,
            cmp_id + 2, cmp_id, cmp_id + 1,
            inst->dest, cmp_id + 2);
        cmp_id += 3;
        break;
    case IR_BIT_AND:
        fprintf(out, "  %%v%d = and i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_BIT_OR:
        fprintf(out, "  %%v%d = or i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_BIT_XOR:
        fprintf(out, "  %%v%d = xor i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_SHL:
        fprintf(out, "  %%v%d = shl i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_SHR:
        fprintf(out, "  %%v%d = ashr i32 %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_MOVE:
        fprintf(out, "  %%v%d = add i32 0, %%v%d\n",
                inst->dest, inst->src1);
        break;

    case IR_ARRAY_LOAD: {
        fprintf(out,
            "  %%ap%d = getelementptr inbounds [%d x i32], [%d x i32]* %%%s, i64 0, i64 %%v%d\n",
            cmp_id, find_var(inst->var_name)->array_size,
            find_var(inst->var_name)->array_size,
            inst->var_name, inst->src1);
        fprintf(out,
            "  %%v%d = load i32, i32* %%ap%d, align 4\n",
            inst->dest, cmp_id);
        cmp_id++;
        break;
    }

    case IR_ARRAY_STORE: {
        VarInfo *v = find_var(inst->var_name);
        fprintf(out,
            "  %%ap%d = getelementptr inbounds [%d x i32], [%d x i32]* %%%s, i64 0, i64 %%v%d\n",
            cmp_id, v->array_size, v->array_size,
            inst->var_name, inst->src1);
        fprintf(out,
            "  store i32 %%v%d, i32* %%ap%d, align 4\n",
            inst->src2, cmp_id);
        cmp_id++;
        break;
    }

    case IR_CHAR_LOAD: {
        int c = cmp_id;
        fprintf(out,
            "  %%sp%d = load i8*, i8** %%%s, align 8\n", c, inst->var_name);
        fprintf(out,
            "  %%cp%d = getelementptr i8, i8* %%sp%d, i32 %%v%d\n",
            c, c, inst->src1);
        fprintf(out,
            "  %%cb%d = load i8, i8* %%cp%d, align 1\n", c, c);
        fprintf(out,
            "  %%v%d = zext i8 %%cb%d to i32\n", inst->dest, c);
        cmp_id++;
        break;
    }

    case IR_PRINT:
        fprintf(out, "  call void @print_int(i32 %%v%d)\n", inst->src1);
        break;

    case IR_PRINT_CHAR: {
        fprintf(out,
            "  %%pc%d = trunc i32 %%v%d to i8\n", cmp_id, inst->src1);
        fprintf(out,
            "  call void @print_char(i8 %%pc%d)\n", cmp_id);
        cmp_id++;
        break;
    }

    case IR_PRINT_STRING: {
        int sid = strlit_id(inst->var_name);
        char reg[32];
        snprintf(reg, sizeof(reg), "%%pslit%d", cmp_id++);
        emit_string_ptr(out, sid, reg);
        fprintf(out, "  call void @print_string(i8* %s)\n", reg);
        break;
    }

    case IR_PRINT_STRING_PTR:
        fprintf(out,
            "  %%pstr%d = load i8*, i8** %%%s, align 8\n",
            cmp_id, inst->var_name);
        fprintf(out,
            "  call void @print_string(i8* %%pstr%d)\n", cmp_id);
        cmp_id++;
        break;

    case IR_LABEL:
        fprintf(out, "%s:\n", inst->label);
        break;

    case IR_JUMP:
        fprintf(out, "  br label %%%s\n", inst->label);
        break;

    case IR_JUMP_IF_FALSE:
        fprintf(out,
            "  %%jf%d = icmp eq i32 %%v%d, 0\n"
            "  br i1 %%jf%d, label %%%s, label %%jf_cont_%d\n",
            cmp_id, inst->src1, cmp_id, inst->label, cmp_id);
        fprintf(out, "jf_cont_%d:\n", cmp_id);
        cmp_id++;
        break;

    case IR_PARAM_STORE: {
        VarInfo *v = find_var(inst->var_name);
        if (!v)
            v = register_var(inst->var_name, 0, 0,
                             inst->src2 == TOKEN_CHAR, 0, 0);
        if (v && v->is_char) {
            fprintf(out,
                "  store i8 %%p%d, i8* %%%s, align 1\n",
                inst->value, inst->var_name);
        } else {
            fprintf(out,
                "  store i32 %%p%d, i32* %%%s, align 4\n",
                inst->value, inst->var_name);
        }
        break;
    }

    case IR_RETURN:
        if (current_func_ret_char) {
            fprintf(out,
                "  %%retc%d = trunc i32 %%v%d to i8\n",
                cmp_id, inst->src1);
            fprintf(out,
                "  ret i8 %%retc%d\n", cmp_id);
            cmp_id++;
        } else {
            fprintf(out, "  ret i32 %%v%d\n", inst->src1);
        }
        break;

    case IR_CAST_I2F:
        fprintf(out,
            "  %%v%d = sitofp i32 %%v%d to float\n",
            inst->dest, inst->src1);
        break;

    case IR_CAST_F2I:
        fprintf(out,
            "  %%v%d = fptosi float %%v%d to i32\n",
            inst->dest, inst->src1);
        break;

    case IR_FADD:
        fprintf(out, "  %%v%d = fadd float %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_FSUB:
        fprintf(out, "  %%v%d = fsub float %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_FMUL:
        fprintf(out, "  %%v%d = fmul float %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;
    case IR_FDIV:
        fprintf(out, "  %%v%d = fdiv float %%v%d, %%v%d\n",
                inst->dest, inst->src1, inst->src2);
        break;

    case IR_STRING_DECL:
    case IR_CHAR_DECL:
    case IR_ARRAY_DECL:
    case IR_STRING_CONST:
    case IR_CHAR_STORE:
        break;

    case IR_ARG:
        if (pending_arg_count < 16)
            pending_args[pending_arg_count++] = inst->src1;
        break;

    case IR_CALL: {
        FuncSig *f = lookup_func_sig(inst->label);
        int ret_char = f && f->ret_char;
        int n = pending_arg_count;
        int base = cmp_id;

        for (int i = 0; i < n; i++) {
            if (f && i < f->nparams && f->param_char[i])
                fprintf(out,
                    "  %%ac%d = trunc i32 %%v%d to i8\n",
                    base + i, pending_args[i]);
        }

        if (ret_char)
            fprintf(out, "  %%callr%d = ", base);
        else
            fprintf(out, "  %%v%d = ", inst->dest);

        fprintf(out, "call %s @%s(",
                ret_char ? "i8" : "i32", inst->label);

        for (int i = 0; i < n; i++) {
            if (f && i < f->nparams && f->param_char[i])
                fprintf(out, "i8 %%ac%d", base + i);
            else
                fprintf(out, "i32 %%v%d", pending_args[i]);
            if (i + 1 < n)
                fprintf(out, ", ");
        }
        fprintf(out, ")\n");

        if (ret_char)
            fprintf(out,
                "  %%v%d = zext i8 %%callr%d to i32\n",
                inst->dest, base);

        cmp_id = base + (n > 0 ? n : 1) + 1;
        pending_arg_count = 0;
        break;
    }

    default:
        break;
    }
}

static int count_function_params(IRInst *start) {
    int n = 0;
    for (IRInst *i = start->next; i; i = i->next) {
        if (i->op == IR_PARAM_STORE)
            n++;
        else if (i->op == IR_CHAR_DECL ||
                 i->op == IR_STRING_DECL ||
                 i->op == IR_ARRAY_DECL)
            continue;
        else
            break;
    }
    return n;
}

static void emit_function(FILE *out, IRInst **cursor) {
    IRInst *inst = *cursor;
    if (!inst || inst->op != IR_LABEL || !is_function_label(inst->label))
        return;

    const char *name = inst->label;
    FuncSig *fs = lookup_func_sig(name);
    int nparams = count_function_params(inst);
    int ret_char = fs && fs->ret_char;
    IRInst *body = inst->next;
    IRInst *end = NULL;

    for (IRInst *s = body; s; s = s->next) {
        if (s->op == IR_LABEL && is_function_label(s->label)) {
            end = s;
            break;
        }
    }

    var_count = 0;
    str_lit_count = 0;
    scan_symbols_range(body, end);

    current_func_ret_char = ret_char;
    pending_arg_count = 0;

    fprintf(out, "define %s @%s(",
            ret_char ? "i8" : "i32", name);
    for (int i = 0; i < nparams; i++) {
        int pchar = fs && i < fs->nparams && fs->param_char[i];
        fprintf(out, "%s %%p%d%s",
                pchar ? "i8" : "i32",
                i, (i + 1 < nparams) ? ", " : "");
    }
    fprintf(out, ") {\nentry:\n");

    for (int i = 0; i < var_count; i++)
        vars[i].emitted = 0;

    emit_var_allocas(out);
    cmp_id = 0;

    int returned = 0;
    for (inst = body; inst != end; inst = inst->next) {
        if (inst->op == IR_RETURN)
            returned = 1;
        emit_instruction(out, inst);
    }

    *cursor = end;
    if (!returned)
        fprintf(out, "  ret %s 0\n", ret_char ? "i8" : "i32");
    fprintf(out, "}\n\n");
}

void codegen_llvm_emit(IRList *ir, FILE *out) {
    cmp_id = 0;
    str_lit_count = 0;
    var_count = 0;

    scan_all_function_sigs(ir);

    emit_header(out);

    /* First pass: collect all string literals */
    for (IRInst *inst = ir->head; inst; inst = inst->next) {
        if (inst->op == IR_STRING_INIT && inst->label)
            strlit_id(inst->label);
        if (inst->op == IR_PRINT_STRING && inst->var_name)
            strlit_id(inst->var_name);
    }

    emit_string_globals(out);
    fprintf(out, "\n");

    IRInst *cur = ir->head;
    while (cur) {
        if (cur->op == IR_LABEL && is_function_label(cur->label)) {
            emit_function(out, &cur);
            if (!cur)
                break;
        } else {
            cur = cur->next;
        }
    }
}
