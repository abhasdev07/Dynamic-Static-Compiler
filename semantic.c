#include "semantic.h"
#include "lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    SEM_INT,
    SEM_FLOAT,
    SEM_CHAR,
    SEM_STRING,
    SEM_VOID,
    SEM_UNKNOWN
} SemType;

typedef struct {
    char name[64];
    SemType type;
    int is_array;
} VarSym;

typedef struct {
    char name[64];
    SemType return_type;
    int param_count;
    SemType param_types[16];
} FuncSym;

typedef struct Scope {
    VarSym vars[128];
    int var_count;
    struct Scope *parent;
} Scope;

static FuncSym func_table[64];
static int func_count;

static Scope scope_make(Scope *parent) {
    Scope s;
    memset(&s, 0, sizeof(s));
    s.parent = parent;
    return s;
}

static void semantic_error(const char *msg) {
    fprintf(stderr, "Semantic Error: %s\n", msg);
    exit(1);
}

static SemType token_to_sem(int tok) {

    switch (tok) {

        case TOKEN_INT:
            return SEM_INT;

        case TOKEN_FLOAT:
            return SEM_FLOAT;

        case TOKEN_CHAR:
            return SEM_CHAR;

        case TOKEN_STRING:
            return SEM_STRING;

        default:
            return SEM_UNKNOWN;
    }
}

static const char *sem_name(SemType t) {

    switch (t) {

        case SEM_INT:
            return "int";

        case SEM_FLOAT:
            return "float";

        case SEM_CHAR:
            return "char";

        case SEM_STRING:
            return "string";

        case SEM_VOID:
            return "void";

        default:
            return "unknown";
    }
}

static int sem_is_numeric(SemType t) {

    return t == SEM_INT ||
           t == SEM_FLOAT ||
           t == SEM_CHAR;
}

static VarSym *scope_lookup(Scope *scope,
                            const char *name) {

    for (Scope *s = scope; s; s = s->parent) {

        for (int i = 0; i < s->var_count; i++) {

            if (strcmp(s->vars[i].name, name) == 0)
                return &s->vars[i];
        }
    }

    return NULL;
}

static void scope_declare(Scope *scope,
                          const char *name,
                          SemType type,
                          int is_array) {

    for (int i = 0; i < scope->var_count; i++) {

        if (strcmp(scope->vars[i].name, name) == 0) {

            char buf[128];

            snprintf(buf,
                     sizeof(buf),
                     "Variable '%s' already declared",
                     name);

            semantic_error(buf);
        }
    }

    if (scope->var_count >= 128)
        semantic_error("Too many variables in scope");

    strncpy(scope->vars[scope->var_count].name,
            name,
            63);

    scope->vars[scope->var_count].name[63] = '\0';

    scope->vars[scope->var_count].type = type;

    scope->vars[scope->var_count].is_array = is_array;

    scope->var_count++;
}

static FuncSym *lookup_function(const char *name) {

    for (int i = 0; i < func_count; i++) {

        if (strcmp(func_table[i].name, name) == 0)
            return &func_table[i];
    }

    return NULL;
}

static void register_function(ASTNode *fn) {

    if (func_count >= 64)
        semantic_error("Too many functions");

    for (int i = 0; i < func_count; i++) {

        if (strcmp(func_table[i].name,
                   fn->function.name) == 0) {

            char buf[128];

            snprintf(buf,
                     sizeof(buf),
                     "Function '%s' already declared",
                     fn->function.name);

            semantic_error(buf);
        }
    }

    FuncSym *f = &func_table[func_count++];

    strncpy(f->name,
            fn->function.name,
            63);

    f->name[63] = '\0';

    f->return_type =
        token_to_sem(fn->function.return_type);

    f->param_count = 0;

    for (ASTList *p = fn->function.params;
         p && f->param_count < 16;
         p = p->next) {

        f->param_types[f->param_count++] =
            token_to_sem(p->stmt->decl.var_type);
    }
}

static SemType check_expr(ASTNode *node,
                          Scope *scope);

static void check_statement(ASTNode *node,
                            Scope *scope,
                            SemType func_ret);

static void check_stmt_list(ASTList *list,
                            Scope *scope,
                            SemType func_ret);

static void check_assign_compatible(SemType lhs,
                                    SemType rhs,
                                    const char *ctx) {

    if (lhs == SEM_UNKNOWN ||
        rhs == SEM_UNKNOWN)
        return;

    if (lhs == rhs)
        return;

    if (lhs == SEM_CHAR &&
        (rhs == SEM_INT || rhs == SEM_CHAR))
        return;

    if (lhs == SEM_INT &&
        (rhs == SEM_CHAR ||
         rhs == SEM_FLOAT))
        return;

    if (lhs == SEM_FLOAT &&
        (rhs == SEM_INT ||
         rhs == SEM_CHAR))
        return;

    char buf[256];

    if (lhs == SEM_INT && rhs == SEM_STRING) {
        if (ctx)
            snprintf(buf, sizeof(buf),
                     "Cannot assign string to int %s", ctx);
        else
            snprintf(buf, sizeof(buf),
                     "Cannot assign string to int");
        semantic_error(buf);
    }

    if (lhs == SEM_CHAR && rhs == SEM_STRING) {
        if (ctx)
            snprintf(buf, sizeof(buf),
                     "Cannot assign string to char %s", ctx);
        else
            snprintf(buf, sizeof(buf),
                     "Cannot assign string to char variable");
        semantic_error(buf);
    }

    if (lhs == SEM_STRING && rhs != SEM_STRING) {
        if (ctx)
            snprintf(buf, sizeof(buf),
                     "Cannot assign %s to string %s",
                     sem_name(rhs), ctx);
        else
            snprintf(buf, sizeof(buf),
                     "Cannot assign %s to string",
                     sem_name(rhs));
        semantic_error(buf);
    }

    if (ctx)
        snprintf(buf,
                 sizeof(buf),
                 "Cannot assign %s to %s %s",
                 sem_name(rhs),
                 sem_name(lhs),
                 ctx);
    else
        snprintf(buf,
                 sizeof(buf),
                 "Cannot assign %s to %s",
                 sem_name(rhs),
                 sem_name(lhs));

    semantic_error(buf);
}

static void check_string_increment(ASTNode *lhs,
                                   ASTNode *rhs,
                                   Scope *scope) {

    if (lhs->type != AST_VAR)
        return;

    VarSym *v = scope_lookup(scope, lhs->var_name);

    if (!v || v->type != SEM_STRING)
        return;

    if (rhs->type == AST_BINARY_OP &&
        (rhs->binop.op == '+' || rhs->binop.op == '-') &&
        rhs->binop.right->type == AST_NUMBER &&
        rhs->binop.right->number == 1) {

        char buf[128];

        snprintf(buf, sizeof(buf),
                 "Cannot increment string variable '%s'",
                 lhs->var_name);

        semantic_error(buf);
    }
}

static SemType lhs_type(ASTNode *lhs,
                        Scope *scope) {

    if (lhs->type == AST_VAR) {

        VarSym *v =
            scope_lookup(scope,
                         lhs->var_name);

        if (!v) {

            char buf[128];

            snprintf(buf,
                     sizeof(buf),
                     "Undeclared variable '%s'",
                     lhs->var_name);

            semantic_error(buf);
        }

        return v->type;
    }

    if (lhs->type == AST_ARRAY_ACCESS) {

        VarSym *v =
            scope_lookup(scope,
                         lhs->array_access.array_name);

        if (!v) {

            char buf[128];

            snprintf(buf,
                     sizeof(buf),
                     "Undeclared variable '%s'",
                     lhs->array_access.array_name);

            semantic_error(buf);
        }

        if (!v->is_array &&
            v->type != SEM_STRING) {

            char buf[128];

            snprintf(buf,
                     sizeof(buf),
                     "Variable '%s' is not indexable",
                     lhs->array_access.array_name);

            semantic_error(buf);
        }

        SemType idx =
            check_expr(lhs->array_access.index,
                       scope);

        if (idx != SEM_INT)
            semantic_error(
                "Array index must be integer");

        return v->type == SEM_STRING
               ? SEM_CHAR
               : SEM_INT;
    }

    return SEM_UNKNOWN;
}

static SemType check_expr(ASTNode *node,
                          Scope *scope) {

    if (!node)
        return SEM_UNKNOWN;

    switch (node->type) {

    case AST_NUMBER:
        return SEM_INT;

    case AST_FLOAT:
        return SEM_FLOAT;

    case AST_STRING:
        return SEM_STRING;

    case AST_VAR: {

        VarSym *v =
            scope_lookup(scope,
                         node->var_name);

        if (!v) {

            char buf[128];

            snprintf(buf,
                     sizeof(buf),
                     "Undeclared variable '%s'",
                     node->var_name);

            semantic_error(buf);
        }

        return v->type;
    }

    case AST_ARRAY_ACCESS: {

        VarSym *v =
            scope_lookup(scope,
                         node->array_access.array_name);

        if (!v) {

            char buf[128];

            snprintf(buf,
                     sizeof(buf),
                     "Undeclared variable '%s'",
                     node->array_access.array_name);

            semantic_error(buf);
        }

        if (!v->is_array &&
            v->type != SEM_STRING) {

            char buf[128];

            snprintf(buf,
                     sizeof(buf),
                     "Variable '%s' is not indexable",
                     node->array_access.array_name);

            semantic_error(buf);
        }

        SemType idx =
            check_expr(node->array_access.index,
                       scope);

        if (idx != SEM_INT)
            semantic_error(
                "Array index must be integer");

        return v->type == SEM_STRING
               ? SEM_CHAR
               : SEM_INT;
    }

    case AST_BINARY_OP: {

        SemType lt =
            check_expr(node->binop.left,
                       scope);

        SemType rt =
            check_expr(node->binop.right,
                       scope);

        int op = node->binop.op;

        if (op == 'a' || op == 'o') {

            if (!sem_is_numeric(lt))
                semantic_error(
                    "Logical operators require numeric operands");

            if (!sem_is_numeric(rt))
                semantic_error(
                    "Logical operators require numeric operands");

            return SEM_INT;
        }

        if (op == '=' ||
            op == '!' ||
            op == '<' ||
            op == '>' ||
            op == 'l' ||
            op == 'g') {

            return SEM_INT;
        }

        if ((op == '+' || op == '-') && lt == SEM_STRING) {
            if (rt == SEM_INT &&
                node->binop.right->type == AST_NUMBER &&
                node->binop.right->number == 1 &&
                node->binop.left->type == AST_VAR) {

                char buf[128];

                snprintf(buf, sizeof(buf),
                         "Cannot increment string variable '%s'",
                         node->binop.left->var_name);

                semantic_error(buf);
            }

            if (op == '+')
                semantic_error(
                    "String concatenation not supported");
        }

        if (op == '+' && rt == SEM_STRING)

            semantic_error(
                "String concatenation not supported");

        if (lt == SEM_STRING ||
            rt == SEM_STRING)

            semantic_error(
                "Invalid operand type for arithmetic operation");

        if (!sem_is_numeric(lt) ||
            !sem_is_numeric(rt))

            semantic_error(
                "Arithmetic requires numeric operands");

        if (lt == SEM_FLOAT ||
            rt == SEM_FLOAT)

            return SEM_FLOAT;

        if (lt == SEM_CHAR && rt == SEM_CHAR)
            return SEM_CHAR;

        return SEM_INT;
    }

    case AST_UNARY_OP: {

        SemType t =
            check_expr(node->unop.operand,
                       scope);

        if (node->unop.op == '!')
            return SEM_INT;

        if (t == SEM_STRING) {
            if (node->unop.op == '+' || node->unop.op == '-')
                semantic_error(
                    "Cannot increment string variable");
            semantic_error(
                "Invalid operand type for unary operation");
        }

        return t == SEM_FLOAT
               ? SEM_FLOAT
               : (t == SEM_CHAR ? SEM_CHAR : SEM_INT);
    }

    case AST_CAST:

        check_expr(node->cast.operand,
                   scope);

        return token_to_sem(
            node->cast.cast_type);

    case AST_TERNARY: {

        check_expr(node->ternary.cond,
                   scope);

        SemType a =
            check_expr(node->ternary.then_expr,
                       scope);

        SemType b =
            check_expr(node->ternary.else_expr,
                       scope);

        if (a != b &&
            a != SEM_UNKNOWN &&
            b != SEM_UNKNOWN)

            semantic_error(
                "Ternary branches have incompatible types");

        return a;
    }

    case AST_FUNCTION_CALL: {

        FuncSym *f =
            lookup_function(
                node->func_call.name);

        if (!f) {

            char buf[128];

            snprintf(buf,
                     sizeof(buf),
                     "Undefined function '%s'",
                     node->func_call.name);

            semantic_error(buf);
        }

        int argc = 0;

        for (ASTList *a =
                 node->func_call.args;
             a;
             a = a->next)

            argc++;

        if (argc != f->param_count) {

            char buf[160];

            snprintf(buf,
                     sizeof(buf),
                     "Function '%s' expects %d arguments but got %d",
                     node->func_call.name,
                     f->param_count,
                     argc);

            semantic_error(buf);
        }

        int idx = 0;

        for (ASTList *a =
                 node->func_call.args;
             a;
             a = a->next, idx++) {

            SemType argt =
                check_expr(a->stmt,
                           scope);

            if (idx < f->param_count)

                check_assign_compatible(
                    f->param_types[idx],
                    argt,
                    "parameter");
        }

        return f->return_type;
    }

    case AST_ASSIGN: {

        SemType lt =
            lhs_type(node->assign.lhs,
                     scope);

        SemType rt =
            check_expr(node->assign.rhs,
                       scope);

        char ctx[80] = "";

        if (node->assign.lhs->type ==
            AST_VAR) {

            snprintf(ctx,
                     sizeof(ctx),
                     "variable '%s'",
                     node->assign.lhs->var_name);
        }

        check_assign_compatible(
            lt,
            rt,
            ctx[0] ? ctx : NULL);

        check_string_increment(
            node->assign.lhs,
            node->assign.rhs,
            scope);

        return rt;
    }

    default:
        return SEM_UNKNOWN;
    }
}

static void check_decl(ASTNode *node,
                       Scope *scope) {

    SemType t =
        token_to_sem(node->decl.var_type);

    scope_declare(scope,
                  node->decl.var_name,
                  t,
                  0);

    if (node->decl.init) {

        SemType it =
            check_expr(node->decl.init,
                       scope);

        char ctx[80];

        snprintf(ctx,
                 sizeof(ctx),
                 "variable '%s'",
                 node->decl.var_name);

        check_assign_compatible(t,
                                it,
                                ctx);
    }
}

static void check_array_decl(ASTNode *node,
                             Scope *scope) {

    SemType t =
        token_to_sem(
            node->array_decl.var_type);

    if (t == SEM_STRING)
        semantic_error(
            "String arrays are not supported");

    scope_declare(scope,
                  node->array_decl.var_name,
                  t,
                  1);

    if (node->array_decl.size_expr)
        check_expr(
            node->array_decl.size_expr,
            scope);
}

static void check_statement(ASTNode *node,
                            Scope *scope,
                            SemType func_ret) {

    if (!node)
        return;

    switch (node->type) {

    case AST_DECL:
        check_decl(node, scope);
        break;

    case AST_ARRAY_DECL:
        check_array_decl(node, scope);
        break;

    case AST_ASSIGN: {

        SemType lt =
            lhs_type(node->assign.lhs,
                     scope);

        SemType rt =
            check_expr(node->assign.rhs,
                       scope);

        char ctx[80] = "";

        if (node->assign.lhs->type ==
            AST_VAR)

            snprintf(ctx,
                     sizeof(ctx),
                     "variable '%s'",
                     node->assign.lhs->var_name);

        check_assign_compatible(
            lt,
            rt,
            ctx[0] ? ctx : NULL);

        check_string_increment(
            node->assign.lhs,
            node->assign.rhs,
            scope);

        break;
    }

    case AST_EXPR_STMT:

        check_expr(node->expr,
                   scope);

        break;

    case AST_BLOCK: {

        Scope block =
            scope_make(scope);

        check_stmt_list(
            node->block.stmts,
            &block,
            func_ret);

        break;
    }

    case AST_IF:

        check_expr(
            node->if_stmt.condition,
            scope);

        check_statement(
            node->if_stmt.then_stmt,
            scope,
            func_ret);

        if (node->if_stmt.else_branch)

            check_statement(
                node->if_stmt.else_branch,
                scope,
                func_ret);

        break;

    case AST_WHILE:

        check_expr(
            node->while_stmt.condition,
            scope);

        check_statement(
            node->while_stmt.do_stmt,
            scope,
            func_ret);

        break;

    case AST_FOR: {

        Scope for_scope =
            scope_make(scope);

        if (node->for_stmt.init) {

            if (node->for_stmt.init->type ==
                AST_DECL)

                check_decl(
                    node->for_stmt.init,
                    &for_scope);

            else

                check_expr(
                    node->for_stmt.init,
                    &for_scope);
        }

        if (node->for_stmt.condition)

            check_expr(
                node->for_stmt.condition,
                &for_scope);

        check_statement(
            node->for_stmt.body,
            &for_scope,
            func_ret);

        if (node->for_stmt.update) {

            if (node->for_stmt.update->type ==
                AST_ASSIGN)

                check_statement(
                    node->for_stmt.update,
                    &for_scope,
                    func_ret);

            else

                check_expr(
                    node->for_stmt.update,
                    &for_scope);
        }

        break;
    }

    case AST_PRINT:

        for (ASTList *a =
                 node->print_stmt.args;
             a;
             a = a->next)

            check_expr(a->stmt,
                       scope);

        break;

    case AST_RETURN: {

        if (!node->expr) {

            if (func_ret != SEM_VOID)

                semantic_error(
                    "Non-void function must return a value");

            break;
        }

        SemType rt =
            check_expr(node->expr,
                       scope);

        if (func_ret != SEM_UNKNOWN)

            check_assign_compatible(
                func_ret,
                rt,
                "return value");

        break;
    }

    default:

        check_expr(node,
                   scope);

        break;
    }
}

static void check_stmt_list(ASTList *list,
                            Scope *scope,
                            SemType func_ret) {

    for (; list; list = list->next)

        check_statement(list->stmt,
                        scope,
                        func_ret);
}

static void check_function(ASTNode *fn,
                           Scope *global) {

    Scope fn_scope =
        scope_make(global);

    for (ASTList *p =
             fn->function.params;
         p;
         p = p->next) {

        ASTNode *param = p->stmt;

        scope_declare(
            &fn_scope,
            param->decl.var_name,
            token_to_sem(
                param->decl.var_type),
            0);
    }

    SemType ret =
        token_to_sem(
            fn->function.return_type);

    check_statement(
        fn->function.body,
        &fn_scope,
        ret);
}

int semantic_check_program(ASTList *program) {

    func_count = 0;

    for (ASTList *p = program;
         p;
         p = p->next) {

        if (p->stmt->type ==
            AST_FUNCTION)

            register_function(
                p->stmt);
    }

    Scope global =
        scope_make(NULL);

    for (ASTList *p = program;
         p;
         p = p->next) {

        if (p->stmt->type ==
            AST_FUNCTION)

            check_function(
                p->stmt,
                &global);

        else

            check_statement(
                p->stmt,
                &global,
                SEM_UNKNOWN);
    }

    printf(
        "Semantic analysis passed.\n");

    return 0;
}