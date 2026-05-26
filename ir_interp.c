#include "ir_interp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TEMPS 4096
#define MAX_VARS 256
#define MAX_ARRAYS 64
#define MAX_CALL_ARGS 16

typedef struct {
    char name[64];
    int value;
} Scalar;

typedef struct {
    char name[64];
    int *data;
    int size;
} ArrayVar;

typedef struct {
    char name[64];
    char *text;
} StrVar;

typedef struct {
    int temps[MAX_TEMPS];
    Scalar scalars[MAX_VARS];
    int scalar_count;
    ArrayVar arrays[MAX_ARRAYS];
    int array_count;
    StrVar strings[MAX_VARS];
    int string_count;
    int call_args[MAX_CALL_ARGS];
    int call_arg_count;
} VM;

static VM vm;

static int get_scalar(const char *name, int *out) {
    for (int i = 0; i < vm.scalar_count; i++)
        if (strcmp(vm.scalars[i].name, name) == 0) {
            *out = vm.scalars[i].value;
            return 1;
        }
    return 0;
}

static void set_scalar(const char *name, int value) {
    for (int i = 0; i < vm.scalar_count; i++) {
        if (strcmp(vm.scalars[i].name, name) == 0) {
            vm.scalars[i].value = value;
            return;
        }
    }
    if (vm.scalar_count < MAX_VARS) {
        strncpy(vm.scalars[vm.scalar_count].name, name, 63);
        vm.scalars[vm.scalar_count].name[63] = '\0';
        vm.scalars[vm.scalar_count].value = value;
        vm.scalar_count++;
    }
}

static ArrayVar *get_array(const char *name) {
    for (int i = 0; i < vm.array_count; i++)
        if (strcmp(vm.arrays[i].name, name) == 0)
            return &vm.arrays[i];
    return NULL;
}

static ArrayVar *ensure_array(const char *name, int size) {
    ArrayVar *a = get_array(name);
    if (!a) {
        if (vm.array_count >= MAX_ARRAYS) return NULL;
        a = &vm.arrays[vm.array_count++];
        strncpy(a->name, name, 63);
        a->name[63] = '\0';
        a->size = size > 0 ? size : 100;
        a->data = calloc((size_t)a->size, sizeof(int));
    }
    return a;
}

static char *get_string(const char *name) {
    for (int i = 0; i < vm.string_count; i++)
        if (strcmp(vm.strings[i].name, name) == 0)
            return vm.strings[i].text;
    return NULL;
}

static void set_string(const char *name, const char *text) {
    for (int i = 0; i < vm.string_count; i++) {
        if (strcmp(vm.strings[i].name, name) == 0) {
            free(vm.strings[i].text);
            vm.strings[i].text = strdup(text);
            return;
        }
    }
    if (vm.string_count < MAX_VARS) {
        strncpy(vm.strings[vm.string_count].name, name, 63);
        vm.strings[vm.string_count].name[63] = '\0';
        vm.strings[vm.string_count].text = strdup(text);
        vm.string_count++;
    }
}

static int temp(int id) {
    if (id < 0 || id >= MAX_TEMPS) return 0;
    return vm.temps[id];
}

static void set_temp(int id, int value) {
    if (id >= 0 && id < MAX_TEMPS)
        vm.temps[id] = value;
}

static IRInst *find_label(IRList *list, const char *name) {
    for (IRInst *i = list->head; i; i = i->next)
        if (i->op == IR_LABEL && i->label && strcmp(i->label, name) == 0)
            return i;
    return NULL;
}

/* Resume execution at the first instruction after a label. */
static IRInst *label_next(IRList *list, const char *name) {
    IRInst *lab = find_label(list, name);
    return lab ? lab->next : NULL;
}

static int is_func_label(const char *label) {
    return label && strchr(label, '_') == NULL;
}

static IRInst *next_func_label(IRInst *start) {
    for (IRInst *i = start->next; i; i = i->next)
        if (i->op == IR_LABEL && is_func_label(i->label))
            return i;
    return NULL;
}

static int exec_inst(IRList *list, IRInst *inst);

static int run_function(IRList *list, const char *name, int argc, int *argv) {
    IRInst *entry = find_label(list, name);
    if (!entry) {
        fprintf(stderr, "[runtime] function '%s' not found\n", name);
        return 0;
    }

    int saved_scalar = vm.scalar_count;
    int saved_string = vm.string_count;
    int saved_array = vm.array_count;

    IRInst *end = next_func_label(entry);
    IRInst *ip = entry->next;

    int pidx = 0;
    while (ip && ip != end && ip->op == IR_PARAM_STORE) {
        if (pidx < argc)
            set_scalar(ip->var_name, argv[pidx++]);
        ip = ip->next;
    }

    int ret = 0;
    while (ip && ip != end) {
        switch (ip->op) {
        case IR_LABEL:
            ip = ip->next;
            continue;

        case IR_RETURN:
            ret = temp(ip->src1);
            goto done;

        case IR_JUMP:
            ip = label_next(list, ip->label);
            continue;

        case IR_JUMP_IF_FALSE:
            if (temp(ip->src1) == 0)
                ip = label_next(list, ip->label);
            else
                ip = ip->next;
            continue;

        default:
            if (exec_inst(list, ip) < 0)
                goto done;
            ip = ip->next;
            break;
        }
    }

done:
    vm.scalar_count = saved_scalar;
    vm.string_count = saved_string;
    vm.array_count = saved_array;
    return ret;
}

static int exec_inst(IRList *list, IRInst *inst) {
    switch (inst->op) {
    case IR_LOAD_CONST:
        set_temp(inst->dest, inst->value);
        break;
    case IR_FLOAD_CONST:
        set_temp(inst->dest, inst->value);
        break;
    case IR_LOAD_VAR: {
        int v;
        if (get_scalar(inst->var_name, &v))
            set_temp(inst->dest, v);
        else
            set_temp(inst->dest, 0);
        break;
    }
    case IR_STORE_VAR:
        set_scalar(inst->var_name, temp(inst->src1));
        break;
    case IR_STRING_INIT:
        set_string(inst->var_name, inst->label);
        break;
    case IR_ARRAY_DECL:
        ensure_array(inst->var_name, inst->value);
        break;
    case IR_ARRAY_LOAD: {
        ArrayVar *a = ensure_array(inst->var_name, 100);
        int idx = temp(inst->src1);
        if (a && idx >= 0 && idx < a->size)
            set_temp(inst->dest, a->data[idx]);
        else
            set_temp(inst->dest, 0);
        break;
    }
    case IR_ARRAY_STORE: {
        ArrayVar *a = ensure_array(inst->var_name, 100);
        int idx = temp(inst->src1);
        if (a && idx >= 0 && idx < a->size)
            a->data[idx] = temp(inst->src2);
        break;
    }
    case IR_CHAR_LOAD: {
        char *s = get_string(inst->var_name);
        int idx = temp(inst->src1);
        unsigned char c = (s && idx >= 0 && (size_t)idx < strlen(s)) ? (unsigned char)s[idx] : 0;
        set_temp(inst->dest, (int)c);
        break;
    }
    case IR_CHAR_STORE: {
        char *s = get_string(inst->var_name);
        int idx = temp(inst->src1);
        if (s && idx >= 0 && (size_t)idx < strlen(s))
            s[idx] = (char)(temp(inst->src2) & 0xFF);
        break;
    }
    case IR_ADD:
        set_temp(inst->dest, temp(inst->src1) + temp(inst->src2));
        break;
    case IR_SUB:
        set_temp(inst->dest, temp(inst->src1) - temp(inst->src2));
        break;
    case IR_MUL:
        set_temp(inst->dest, temp(inst->src1) * temp(inst->src2));
        break;
    case IR_DIV: {
        int b = temp(inst->src2);
        set_temp(inst->dest, b != 0 ? temp(inst->src1) / b : 0);
        break;
    }
    case IR_MOD: {
        int b = temp(inst->src2);
        set_temp(inst->dest, b != 0 ? temp(inst->src1) % b : 0);
        break;
    }
    case IR_NEG:
        set_temp(inst->dest, -temp(inst->src1));
        break;
    case IR_LOG_NOT:
        set_temp(inst->dest, temp(inst->src1) == 0 ? 1 : 0);
        break;
    case IR_BIT_NOT:
        set_temp(inst->dest, ~temp(inst->src1));
        break;
    case IR_EQ:
        set_temp(inst->dest, temp(inst->src1) == temp(inst->src2) ? 1 : 0);
        break;
    case IR_NEQ:
        set_temp(inst->dest, temp(inst->src1) != temp(inst->src2) ? 1 : 0);
        break;
    case IR_LT:
        set_temp(inst->dest, temp(inst->src1) < temp(inst->src2) ? 1 : 0);
        break;
    case IR_GT:
        set_temp(inst->dest, temp(inst->src1) > temp(inst->src2) ? 1 : 0);
        break;
    case IR_LE:
        set_temp(inst->dest, temp(inst->src1) <= temp(inst->src2) ? 1 : 0);
        break;
    case IR_GE:
        set_temp(inst->dest, temp(inst->src1) >= temp(inst->src2) ? 1 : 0);
        break;
    case IR_AND:
        set_temp(inst->dest, temp(inst->src1) && temp(inst->src2) ? 1 : 0);
        break;
    case IR_OR:
        set_temp(inst->dest, temp(inst->src1) || temp(inst->src2) ? 1 : 0);
        break;
    case IR_BIT_AND:
        set_temp(inst->dest, temp(inst->src1) & temp(inst->src2));
        break;
    case IR_BIT_OR:
        set_temp(inst->dest, temp(inst->src1) | temp(inst->src2));
        break;
    case IR_BIT_XOR:
        set_temp(inst->dest, temp(inst->src1) ^ temp(inst->src2));
        break;
    case IR_SHL:
        set_temp(inst->dest, temp(inst->src1) << temp(inst->src2));
        break;
    case IR_SHR:
        set_temp(inst->dest, temp(inst->src1) >> temp(inst->src2));
        break;
    case IR_MOVE:
        set_temp(inst->dest, temp(inst->src1));
        break;
    case IR_CAST_F2I:
        set_temp(inst->dest, temp(inst->src1));
        break;
    case IR_CAST_I2F:
        set_temp(inst->dest, temp(inst->src1));
        break;
    case IR_ARG:
        if (vm.call_arg_count < MAX_CALL_ARGS)
            vm.call_args[vm.call_arg_count++] = temp(inst->src1);
        break;
    case IR_CALL: {
        int ret = run_function(list, inst->label, vm.call_arg_count, vm.call_args);
        vm.call_arg_count = 0;
        set_temp(inst->dest, ret);
        break;
    }
    case IR_PRINT:
        printf("%d\n", temp(inst->src1));
        fflush(stdout);
        break;
    case IR_PRINT_STRING:
        printf("%s\n", inst->var_name ? inst->var_name : "");
        fflush(stdout);
        break;
    case IR_PRINT_STRING_PTR: {
        /* src1 is temp holding nothing useful; skip */
        (void)inst;
        break;
    }
    case IR_PRINT_CHAR:
        printf("%c\n", (char)(temp(inst->src1) & 0xFF));
        fflush(stdout);
        break;
    case IR_JUMP:
    case IR_JUMP_IF_FALSE:
    case IR_LABEL:
    case IR_PARAM_STORE:
    case IR_RETURN:
        /* handled by instruction-pointer loop in run_function */
        break;
    case IR_STRING_DECL:
    case IR_STRING_CONST:
        break;
    default:
        break;
    }
    return 0;
}

void ir_interp_run(IRList *list) {
    for (int i = 0; i < vm.array_count; i++)
        free(vm.arrays[i].data);
    for (int i = 0; i < vm.string_count; i++)
        free(vm.strings[i].text);

    memset(&vm, 0, sizeof(vm));
    vm.call_arg_count = 0;

    int ret = run_function(list, "main", 0, NULL);
    if (find_label(list, "main"))
        printf("[main returned] %d\n", ret);
    else
        printf("(no main() function — skipping execution)\n");
    fflush(stdout);
}
