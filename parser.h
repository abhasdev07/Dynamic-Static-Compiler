#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include <stdio.h>

typedef enum {
    AST_NUMBER,
    AST_FOR,
    AST_STRING,
    AST_BINARY_OP,
    AST_VAR,
    AST_FLOAT,
    AST_ASSIGN,
    AST_TERNARY,
    AST_EXPR_STMT,
    AST_DECL,
    AST_UNARY_OP,
    AST_BLOCK,
    AST_IF,
    AST_WHILE,
    AST_PRINT,
    AST_CAST,
    AST_ARRAY_DECL,
    AST_ARRAY_ACCESS,
    AST_RETURN,
    AST_FUNCTION_CALL,
    AST_FUNCTION
} ASTNodeType;

typedef struct ASTNode ASTNode;

typedef struct ASTList {
    ASTNode *stmt;
    struct ASTList *next;
} ASTList;

typedef struct ASTNode {
    ASTNodeType type;
    union {
        int number;
        float float_number;
        char *var_name;
        char *string_value;

        struct {
            int op;
            ASTNode *left;
            ASTNode *right;
        } binop;

        struct {
            ASTNode *lhs;
            ASTNode *rhs;
        } assign;

        struct {
            int cast_type;
            ASTNode *operand;
        } cast;

        struct {
            int var_type;
            char *var_name;
            ASTNode *init;
        } decl;

        struct {
            ASTList *stmts;
        } block;

        struct {
            ASTNode *condition;
            ASTNode *then_stmt;
            ASTNode *else_branch;
        } if_stmt;

        struct {
            ASTNode *condition;
            ASTNode *do_stmt;
        } while_stmt;

        struct {
            ASTNode *init;
            ASTNode *condition;
            ASTNode *update;
            ASTNode *body;
        } for_stmt;

        struct {
            int op;
            ASTNode *operand;
        } unop;

        struct {
            ASTList *args;
        } print_stmt;

        struct {
            ASTNode *cond;
            ASTNode *then_expr;
            ASTNode *else_expr;
        } ternary;

        struct {
            int var_type;
            char *var_name;
            ASTNode *size_expr;
        } array_decl;

        struct {
            char *array_name;
            ASTNode *index;
        } array_access;

        struct {
            int return_type;
            char *name;
            ASTList *params;
            ASTNode *body;
        } function;

        ASTNode *expr;
        ASTNode *return_expr;
        struct {
            char *name;
            struct ASTList *args;
        } func_call;

    };
} ASTNode;

void parser_init(FILE *src);
ASTList *parse_program(void);
ASTNode *parse_variable(void);
void ast_free(ASTNode *node);
void ast_list_free(ASTList *list);
ASTNode *new_node(ASTNodeType type);
void ast_print(ASTNode *node, int indent);
void ast_print_program(ASTList *program);

#endif
