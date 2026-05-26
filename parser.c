#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"
#include "lexer.h"


ASTNode *parse_statement(void);
ASTNode *parse_declaration(void);
ASTNode *parse_assignment(void);
ASTNode *parse_if_statement(void);
ASTNode *parse_while_statement(void);
ASTNode *parse_for_statement(void);
ASTNode *parse_print_statement(void);
ASTNode *parse_return_statement(void);
ASTNode *parse_block(void);
ASTNode *parse_assignment_from_variable(ASTNode *lhs);
static Token current_token;

void parser_init(FILE *src) {
    lexer_init(src);
    current_token = lexer_next_token();
}

static void advance() {
    current_token = lexer_next_token();
}

static void expect(TokenType type) {
    if (current_token.type != type) {
        fprintf(stderr, "Syntax error: expected %d but got %d\n", type, current_token.type);
        exit(EXIT_FAILURE);
    }
    advance();
}

ASTNode *new_node(ASTNodeType type) {
    ASTNode *node = malloc(sizeof(ASTNode));
    if (!node) {
        perror("new_node");
        exit(EXIT_FAILURE);
    }
    node->type = type;
    return node;
}

ASTNode *parse_expression();
ASTNode *parse_ternary();
ASTNode *parse_or();
ASTNode *parse_and();
ASTNode *parse_equality();
ASTNode *parse_relational();
ASTNode *parse_bitwise();
ASTNode *parse_additive();
ASTNode *parse_term();
ASTNode *parse_factor();
ASTNode *parse_function(int return_type, char *name);

ASTNode *parse_function(int return_type, char *name) {
    ASTNode *node = new_node(AST_FUNCTION);
    node->function.name = name;
    node->function.return_type = return_type;
    node->function.params = NULL;
    ASTList *tail = NULL;
    expect(TOKEN_LPAREN);
    while (current_token.type != TOKEN_RPAREN) {
        int type = current_token.type;
        advance();
        char *param_name = strdup(current_token.text);
        expect(TOKEN_IDENTIFIER);
        ASTNode *param = new_node(AST_DECL);
        param->decl.var_type = type;
        param->decl.var_name = param_name;
        param->decl.init = NULL;

        ASTList *arg = malloc(sizeof(ASTList));
        arg->stmt = param;
        arg->next = NULL;
        if (tail) tail->next = arg;
        else node->function.params = arg;
        tail = arg;

        if (current_token.type == TOKEN_COMMA) advance();
    }
    expect(TOKEN_RPAREN);
    node->function.body = parse_block();
    return node;
}

ASTList *parse_program(void) {
    ASTList *head = NULL, *tail = NULL;
    while (current_token.type != TOKEN_EOF) {
        ASTNode *stmt = NULL;

        if (current_token.type == TOKEN_INT || current_token.type == TOKEN_FLOAT || current_token.type == TOKEN_CHAR || current_token.type == TOKEN_STRING) {
            int type = current_token.type;
            advance();

            if (current_token.type != TOKEN_IDENTIFIER) {
                fprintf(stderr, "Expected identifier after type\n");
                exit(1);
            }

            char *name = strdup(current_token.text);
            advance();

            if (current_token.type == TOKEN_LPAREN) {
                stmt = parse_function(type, name);
            } else if (current_token.type == TOKEN_LBRACKET) {
                advance();
                ASTNode *size_expr = parse_expression();
                expect(TOKEN_RBRACKET);
                expect(TOKEN_SEMICOLON);
                ASTNode *decl = new_node(AST_ARRAY_DECL);
                decl->array_decl.var_type = type;
                decl->array_decl.var_name = name;
                decl->array_decl.size_expr = size_expr;
                stmt = decl;
            } else {
                TokenType assign_or_semicolon = current_token.type;

                ASTNode *decl = new_node(AST_DECL);
                decl->decl.var_type = type;
                decl->decl.var_name = name;
                decl->decl.init = NULL;

                if (assign_or_semicolon == TOKEN_ASSIGN) {
                    advance();
                    decl->decl.init = parse_expression();
                }

                expect(TOKEN_SEMICOLON);
                stmt = decl;
            }
        } else {
            stmt = parse_statement();
        }

        ASTList *node = malloc(sizeof(ASTList));
        node->stmt = stmt;
        node->next = NULL;
        if (tail) tail->next = node;
        else head = node;
        tail = node;
    }
    return head;
}

ASTNode *parse_statement() {
    if (current_token.type == TOKEN_INT || current_token.type == TOKEN_CHAR || current_token.type == TOKEN_FLOAT || current_token.type == TOKEN_STRING)
        return parse_declaration();
    if (current_token.type == TOKEN_IF) return parse_if_statement();
    if (current_token.type == TOKEN_WHILE) return parse_while_statement();
    if (current_token.type == TOKEN_FOR) return parse_for_statement();
    if (current_token.type == TOKEN_PRINT) return parse_print_statement();
    if (current_token.type == TOKEN_RETURN) return parse_return_statement();
    if (current_token.type == TOKEN_LBRACE) return parse_block();

    // Step 1: parse LHS expression
    ASTNode *lhs = parse_expression();

    // Step 2: if it's a variable or array access AND followed by an assignment operator
    if ((lhs->type == AST_VAR || lhs->type == AST_ARRAY_ACCESS) && (
        current_token.type == TOKEN_ASSIGN ||
        current_token.type == TOKEN_PLUS_ASSIGN ||
        current_token.type == TOKEN_MINUS_ASSIGN ||
        current_token.type == TOKEN_MUL_ASSIGN ||
        current_token.type == TOKEN_DIV_ASSIGN ||
        current_token.type == TOKEN_MOD_ASSIGN ||
        current_token.type == TOKEN_AND_ASSIGN ||
        current_token.type == TOKEN_OR_ASSIGN ||
        current_token.type == TOKEN_XOR_ASSIGN ||
        current_token.type == TOKEN_SHL_ASSIGN ||
        current_token.type == TOKEN_SHR_ASSIGN)) {
        
        return parse_assignment_from_variable(lhs);  // Handles semicolon
    }

    // Otherwise, it's just a standalone expression
    expect(TOKEN_SEMICOLON);
    ASTNode *stmt = new_node(AST_EXPR_STMT);
    stmt->expr = lhs;
    return stmt;
}

ASTNode *parse_block() {
    expect(TOKEN_LBRACE);
    ASTList *stmts = NULL, *tail = NULL;
    while (current_token.type != TOKEN_RBRACE) {
        ASTNode *stmt = parse_statement();
        ASTList *node = malloc(sizeof(ASTList));
        node->stmt = stmt;
        node->next = NULL;
        if (tail) tail->next = node;
        else stmts = node;
        tail = node;
    }
    expect(TOKEN_RBRACE);
    ASTNode *block = new_node(AST_BLOCK);
    block->block.stmts = stmts;
    return block;
}

ASTNode *parse_variable() {
    if (current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier\n" );
        exit(EXIT_FAILURE);
    }
    ASTNode *node = new_node(AST_VAR);
    node->var_name = strdup(current_token.text);
    advance();
    return node;
}

ASTNode *parse_expression() {
    return parse_ternary();
}


ASTNode *parse_ternary() {
    ASTNode *cond = parse_or();

    if (current_token.type == TOKEN_QUESTION) {
        advance();  // consume '?'
        ASTNode *then_expr = parse_expression();
        expect(TOKEN_COLON);
        ASTNode *else_expr = parse_expression();

        ASTNode *node = new_node(AST_TERNARY);
        node->ternary.cond = cond;
        node->ternary.then_expr = then_expr;
        node->ternary.else_expr = else_expr;
        return node;
    }

    return cond;
}


ASTNode *parse_or() {
    ASTNode *left = parse_and();
    while (current_token.type == TOKEN_OR) {
        int op = 'o';  /* logical OR */
        advance();
        ASTNode *right = parse_and();
        ASTNode *node = new_node(AST_BINARY_OP);
        node->binop.op = op;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

ASTNode *parse_and() {
    ASTNode *left = parse_equality();
    while (current_token.type == TOKEN_AND) {
        int op = 'a';  /* logical AND */
        advance();
        ASTNode *right = parse_equality();
        ASTNode *node = new_node(AST_BINARY_OP);
        node->binop.op = op;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

ASTNode *parse_equality() {
    ASTNode *left = parse_relational();
    while (current_token.type == TOKEN_EQ || current_token.type == TOKEN_NEQ) {
        int op = (current_token.type == TOKEN_EQ) ? '=' : '!';
        advance();
        ASTNode *right = parse_relational();
        ASTNode *node = new_node(AST_BINARY_OP);
        node->binop.op = op;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

ASTNode *parse_relational() {
    ASTNode *left = parse_bitwise();
    while (current_token.type == TOKEN_LT || current_token.type == TOKEN_GT ||
           current_token.type == TOKEN_LE || current_token.type == TOKEN_GE) {
        int op;
        switch (current_token.type) {
            case TOKEN_LT: op = '<'; break;
            case TOKEN_GT: op = '>'; break;
            case TOKEN_LE: op = 'l'; break;
            case TOKEN_GE: op = 'g'; break;
            default:
                fprintf(stderr, "Unexpected token %d in relational operator\n", current_token.type);
                exit(EXIT_FAILURE);
        }
        advance();
        ASTNode *right = parse_bitwise();
        ASTNode *node = new_node(AST_BINARY_OP);
        node->binop.op = op;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

ASTNode *parse_bitwise() {
    ASTNode *left = parse_additive();
    while (current_token.type == TOKEN_BIT_AND || current_token.type == TOKEN_BIT_OR ||
           current_token.type == TOKEN_BIT_XOR || current_token.type == TOKEN_SHL || current_token.type == TOKEN_SHR) {
        int op;
        switch (current_token.type) {
            case TOKEN_BIT_AND: op = '&'; break;
            case TOKEN_BIT_OR:  op = '|'; break;
            case TOKEN_BIT_XOR: op = '^'; break;
            case TOKEN_SHL:     op = 'L'; break;
            case TOKEN_SHR:     op = 'R'; break;
            default: break;
        }
        advance();
        ASTNode *right = parse_additive();
        ASTNode *node = new_node(AST_BINARY_OP);
        node->binop.op = op;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

ASTNode *parse_additive() {
    ASTNode *left = parse_term();
    while (current_token.type == TOKEN_PLUS || current_token.type == TOKEN_MINUS) {
        int op = (current_token.type == TOKEN_PLUS) ? '+' : '-';
        advance();
        ASTNode *right = parse_term();
        ASTNode *node = new_node(AST_BINARY_OP);
        node->binop.op = op;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

ASTNode *parse_term() {
    ASTNode *left = parse_factor();
    while (current_token.type == TOKEN_STAR || current_token.type == TOKEN_SLASH || current_token.type == TOKEN_PERCENT) {
        int op;
        switch (current_token.type) {
            case TOKEN_STAR: op = '*'; break;
            case TOKEN_SLASH: op = '/'; break;
            case TOKEN_PERCENT: op = '%'; break;
            default:
                fprintf(stderr, "Unexpected token %d in term operator\n", current_token.type);
                exit(EXIT_FAILURE);
        }
        advance();
        ASTNode *right = parse_factor();
        ASTNode *node = new_node(AST_BINARY_OP);
        node->binop.op = op;
        node->binop.left = left;
        node->binop.right = right;
        left = node;
    }
    return left;
}

ASTNode *parse_factor() {
    if (current_token.type == TOKEN_INC || current_token.type == TOKEN_DEC ||
        current_token.type == TOKEN_MINUS || current_token.type == TOKEN_LOG_NOT || current_token.type == TOKEN_BIT_NOT) {
        int op;
        TokenType t = current_token.type;
        switch (t) {
            case TOKEN_INC: op = '+'; break;
            case TOKEN_DEC: op = '-'; break;
            case TOKEN_MINUS: op = '-'; break;
            case TOKEN_LOG_NOT: op = '!'; break;
            case TOKEN_BIT_NOT: op = '~'; break;
            default:
                fprintf(stderr, "Unexpected token %d in unary operator", t);
                exit(EXIT_FAILURE);
        }
        advance();
        ASTNode *operand = parse_factor();

        if (t == TOKEN_INC || t == TOKEN_DEC) {
            ASTNode *load = new_node(AST_VAR);
            load->var_name = strdup(operand->var_name);

            ASTNode *rhs = new_node(AST_BINARY_OP);
            rhs->binop.left = load;
            rhs->binop.right = new_node(AST_NUMBER);
            rhs->binop.right->number = 1;
            rhs->binop.op = op;

            ASTNode *assign = new_node(AST_ASSIGN);
            assign->assign.lhs = operand;
            assign->assign.rhs = rhs;
            return assign;
        }

        ASTNode *node = new_node(AST_UNARY_OP);
        node->unop.op = op;
        node->unop.operand = operand;
        return node;
    }

    if (current_token.type == TOKEN_LPAREN) {
        advance();

        // Check for cast: (type)expr
        if (current_token.type == TOKEN_INT || current_token.type == TOKEN_FLOAT || current_token.type == TOKEN_CHAR || current_token.type == TOKEN_STRING) {
            int cast_type = current_token.type;
            advance();
            expect(TOKEN_RPAREN);

            ASTNode *operand = parse_factor(); // Parse what’s being cast

            ASTNode *cast = new_node(AST_CAST);
            cast->cast.cast_type = cast_type;
            cast->cast.operand = operand;
            return cast;
        }

        // Fallback: treat as parenthesized expression
        ASTNode *expr = parse_expression();
        expect(TOKEN_RPAREN);
        return expr;
    }

    ASTNode *node = NULL;

    if (current_token.type == TOKEN_NUMBER || current_token.type == TOKEN_CHAR_LITERAL) {
        node = new_node(AST_NUMBER);
        node->number = current_token.value;
        advance();
    } else if (current_token.type == TOKEN_IDENTIFIER) {
        node = parse_variable();

        // Function call
        if (current_token.type == TOKEN_LPAREN) {
            advance();
            ASTList *args = NULL, *tail = NULL;
            while (current_token.type != TOKEN_RPAREN) {
                ASTNode *arg = parse_expression();
                ASTList *arg_node = malloc(sizeof(ASTList));
                arg_node->stmt = arg;
                arg_node->next = NULL;
                if (tail) tail->next = arg_node;
                else args = arg_node;
                tail = arg_node;
                if (current_token.type == TOKEN_COMMA) advance();
            }
            expect(TOKEN_RPAREN);

            ASTNode *call = new_node(AST_FUNCTION_CALL);
            call->func_call.name = strdup(node->var_name);
            call->func_call.args = args;
            return call;
        }

        // Array access
        if (current_token.type == TOKEN_LBRACKET) {
            advance();
            ASTNode *index = parse_expression();
            expect(TOKEN_RBRACKET);

            ASTNode *access = new_node(AST_ARRAY_ACCESS);
            access->array_access.array_name = node->var_name;
            access->array_access.index = index;
            free(node);
            node = access;
        }

        // Postfix ++/--
        if (current_token.type == TOKEN_INC || current_token.type == TOKEN_DEC) {
            int op = (current_token.type == TOKEN_INC) ? '+' : '-';
            advance();

            ASTNode *load = new_node(AST_VAR);
            load->var_name = strdup(node->type == AST_VAR ? node->var_name : ((node->type == AST_ARRAY_ACCESS) ? node->array_access.array_name : ""));

            ASTNode *rhs = new_node(AST_BINARY_OP);
            rhs->binop.left = load;
            rhs->binop.right = new_node(AST_NUMBER);
            rhs->binop.right->number = 1;
            rhs->binop.op = op;

            ASTNode *assign = new_node(AST_ASSIGN);
            assign->assign.lhs = node;
            assign->assign.rhs = rhs;
            return assign;
        }
    } else if (current_token.type == TOKEN_FLOAT_LITERAL) {
        node = new_node(AST_FLOAT);
        node->float_number = current_token.float_value;
        advance();
    } else if (current_token.type == TOKEN_STRING_LITERAL) {
        node = new_node(AST_STRING);
        node->string_value = strdup(current_token.text);
        advance();
    } else {
        fprintf(stderr, "Unexpected token in factor: %d\n", current_token.type);
        exit(EXIT_FAILURE);
    }

    return node;
}

ASTNode *parse_assignment() {
    ASTNode *lhs = parse_factor();

    TokenType assign_type = current_token.type;

    if (assign_type == TOKEN_ASSIGN || 
        assign_type == TOKEN_PLUS_ASSIGN || assign_type == TOKEN_MINUS_ASSIGN ||
        assign_type == TOKEN_MUL_ASSIGN || assign_type == TOKEN_DIV_ASSIGN || assign_type == TOKEN_MOD_ASSIGN ||
        assign_type == TOKEN_AND_ASSIGN || assign_type == TOKEN_OR_ASSIGN || assign_type == TOKEN_XOR_ASSIGN ||
        assign_type == TOKEN_SHL_ASSIGN || assign_type == TOKEN_SHR_ASSIGN) {

        advance();
        ASTNode *rhs = parse_expression();

        if (assign_type != TOKEN_ASSIGN) {
            int op;
            switch (assign_type) {
                case TOKEN_PLUS_ASSIGN: op = '+'; break;
                case TOKEN_MINUS_ASSIGN: op = '-'; break;
                case TOKEN_MUL_ASSIGN: op = '*'; break;
                case TOKEN_DIV_ASSIGN: op = '/'; break;
                case TOKEN_MOD_ASSIGN: op = '%'; break;
                case TOKEN_AND_ASSIGN: op = '&'; break;
                case TOKEN_OR_ASSIGN:  op = '|'; break;
                case TOKEN_XOR_ASSIGN: op = '^'; break;
                case TOKEN_SHL_ASSIGN: op = 'L'; break;
                case TOKEN_SHR_ASSIGN: op = 'R'; break;
                default: op = '?'; break;
            }

            ASTNode *load_lhs = new_node(AST_VAR);
            load_lhs->var_name = strdup(lhs->var_name);

            ASTNode *binop = new_node(AST_BINARY_OP);
            binop->binop.left = load_lhs;
            binop->binop.right = rhs;
            binop->binop.op = op;

            ASTNode *assign = new_node(AST_ASSIGN);
            assign->assign.lhs = lhs;
            assign->assign.rhs = binop;
            expect(TOKEN_SEMICOLON);
            return assign;
        }

        ASTNode *assign = new_node(AST_ASSIGN);
        assign->assign.lhs = lhs;
        assign->assign.rhs = rhs;
        expect(TOKEN_SEMICOLON);
        return assign;
    }

    fprintf(stderr, "Expected assignment operator\n");
    exit(EXIT_FAILURE);
}

ASTNode *parse_declaration() {
    if (current_token.type != TOKEN_INT && current_token.type != TOKEN_CHAR && current_token.type != TOKEN_FLOAT && current_token.type != TOKEN_STRING) {
        fprintf(stderr, "Expected type keyword (int or char)\n");
        exit(EXIT_FAILURE);
    }
    int var_type = current_token.type;
    advance();

    if (current_token.type != TOKEN_IDENTIFIER) {
        fprintf(stderr, "Expected identifier in declaration\n");
        exit(EXIT_FAILURE);
    }

    char *name = strdup(current_token.text);
    advance();

    if (current_token.type == TOKEN_LBRACKET) {
        advance();
        ASTNode *size_expr = parse_expression();
        expect(TOKEN_RBRACKET);
        expect(TOKEN_SEMICOLON);

        ASTNode *decl = new_node(AST_ARRAY_DECL);
        decl->array_decl.var_type = var_type;
        decl->array_decl.var_name = name;
        decl->array_decl.size_expr = size_expr;
        return decl;
    }

    ASTNode *init = NULL;
    if (current_token.type == TOKEN_ASSIGN) {
        advance();
        init = parse_expression();
    }

    expect(TOKEN_SEMICOLON);

    ASTNode *decl = new_node(AST_DECL);
    decl->decl.var_type = var_type;
    decl->decl.var_name = name;
    decl->decl.init = init;
    return decl;
}

ASTNode *parse_if_statement() {
    expect(TOKEN_IF);
    expect(TOKEN_LPAREN);
    ASTNode *cond = parse_expression();
    expect(TOKEN_RPAREN);
    ASTNode *then_stmt = parse_statement();
    ASTNode *stmt = new_node(AST_IF);
    stmt->if_stmt.condition = cond;
    stmt->if_stmt.then_stmt = then_stmt;
    stmt->if_stmt.else_branch = NULL;
    if (current_token.type == TOKEN_ELSE) {
        advance();
        stmt->if_stmt.else_branch = parse_statement();
    }
    return stmt;
}

ASTNode *parse_while_statement() {
    expect(TOKEN_WHILE);
    expect(TOKEN_LPAREN);
    ASTNode *cond = parse_expression();
    expect(TOKEN_RPAREN);
    ASTNode *body = parse_statement();
    ASTNode *stmt = new_node(AST_WHILE);
    stmt->while_stmt.condition = cond;
    stmt->while_stmt.do_stmt = body;
    return stmt;
}

ASTNode *parse_for_statement() {
    expect(TOKEN_FOR);
    expect(TOKEN_LPAREN);

    ASTNode *init = NULL;
    ASTNode *cond = NULL;
    ASTNode *update = NULL;

    // Parse initialization
    if (current_token.type == TOKEN_INT || current_token.type == TOKEN_CHAR || current_token.type == TOKEN_FLOAT) {
        init = parse_declaration();
    } else if (current_token.type != TOKEN_SEMICOLON) {
        init = parse_expression();
        expect(TOKEN_SEMICOLON);
    } else {
        expect(TOKEN_SEMICOLON);  // empty init
    }

    // Parse condition
    if (current_token.type != TOKEN_SEMICOLON) {
        cond = parse_expression();
    }
    expect(TOKEN_SEMICOLON);

    // Parse update — this MUST be an expression, NOT an assignment statement
    if (current_token.type != TOKEN_RPAREN) {
        // Parse left side
        ASTNode *lhs = parse_factor();

        if (current_token.type == TOKEN_ASSIGN ||
            current_token.type == TOKEN_PLUS_ASSIGN ||
            current_token.type == TOKEN_MINUS_ASSIGN) {
            //TokenType assign_type = current_token.type;
            advance();
            ASTNode *rhs = parse_expression();

            ASTNode *assign = new_node(AST_ASSIGN);
            assign->assign.lhs = lhs;
            assign->assign.rhs = rhs;
            update = assign;
        } else {
            // fallback — treat as general expression
            update = lhs;
        }
    }

    expect(TOKEN_RPAREN);
    ASTNode *body = parse_statement();

    ASTNode *stmt = new_node(AST_FOR);
    stmt->for_stmt.init = init;
    stmt->for_stmt.condition = cond;
    stmt->for_stmt.update = update;
    stmt->for_stmt.body = body;
    return stmt;
}

ASTNode *parse_print_statement() {
    expect(TOKEN_PRINT);
    expect(TOKEN_LPAREN);

    ASTList *args = NULL, *tail = NULL;
    if (current_token.type != TOKEN_RPAREN) {
        do {
            ASTNode *arg = parse_expression();
            ASTList *node = malloc(sizeof(ASTList));
            node->stmt = arg;
            node->next = NULL;
            if (tail) tail->next = node;
            else args = node;
            tail = node;

            if (current_token.type == TOKEN_COMMA)
                advance();
            else
                break;
        } while (current_token.type != TOKEN_RPAREN);
    }

    expect(TOKEN_RPAREN);
    expect(TOKEN_SEMICOLON);

    ASTNode *stmt = new_node(AST_PRINT);
    stmt->print_stmt.args = args;
    return stmt;
}

ASTNode *parse_return_statement() {
    expect(TOKEN_RETURN);
    ASTNode *expr = parse_expression();
    expect(TOKEN_SEMICOLON);
    ASTNode *stmt = new_node(AST_RETURN);
    stmt->expr = expr;
    return stmt;
}

static void ast_indent(int n) {
    for (int i = 0; i < n; i++) printf("  ");
}

void ast_print(ASTNode *node, int indent) {
    if (!node) return;
    ast_indent(indent);
    switch (node->type) {
        case AST_NUMBER: printf("NUMBER %d\n", node->number); break;
        case AST_FLOAT: printf("FLOAT %f\n", node->float_number); break;
        case AST_STRING: printf("STRING \"%s\"\n", node->string_value); break;
        case AST_VAR: printf("VAR %s\n", node->var_name); break;
        case AST_BINARY_OP:
            printf("BINOP '%c'\n", node->binop.op);
            ast_print(node->binop.left, indent + 1);
            ast_print(node->binop.right, indent + 1);
            break;
        case AST_UNARY_OP:
            printf("UNOP '%c'\n", node->unop.op);
            ast_print(node->unop.operand, indent + 1);
            break;
        case AST_ASSIGN:
            printf("ASSIGN\n");
            ast_print(node->assign.lhs, indent + 1);
            ast_print(node->assign.rhs, indent + 1);
            break;
        case AST_DECL:
            printf("DECL type=%d name=%s\n", node->decl.var_type, node->decl.var_name);
            if (node->decl.init) ast_print(node->decl.init, indent + 1);
            break;
        case AST_ARRAY_DECL:
            printf("ARRAY_DECL type=%d name=%s\n", node->array_decl.var_type, node->array_decl.var_name);
            ast_print(node->array_decl.size_expr, indent + 1);
            break;
        case AST_ARRAY_ACCESS:
            printf("ARRAY_ACCESS %s\n", node->array_access.array_name);
            ast_print(node->array_access.index, indent + 1);
            break;
        case AST_FUNCTION:
            printf("FUNCTION %s (ret=%d)\n", node->function.name, node->function.return_type);
            ast_print(node->function.body, indent + 1);
            break;
        case AST_FUNCTION_CALL:
            printf("CALL %s\n", node->func_call.name);
            for (ASTList *a = node->func_call.args; a; a = a->next)
                ast_print(a->stmt, indent + 1);
            break;
        case AST_IF:
            printf("IF\n");
            ast_print(node->if_stmt.condition, indent + 1);
            ast_print(node->if_stmt.then_stmt, indent + 1);
            if (node->if_stmt.else_branch) ast_print(node->if_stmt.else_branch, indent + 1);
            break;
        case AST_WHILE:
            printf("WHILE\n");
            ast_print(node->while_stmt.condition, indent + 1);
            ast_print(node->while_stmt.do_stmt, indent + 1);
            break;
        case AST_FOR:
            printf("FOR\n");
            ast_print(node->for_stmt.init, indent + 1);
            ast_print(node->for_stmt.condition, indent + 1);
            ast_print(node->for_stmt.update, indent + 1);
            ast_print(node->for_stmt.body, indent + 1);
            break;
        case AST_PRINT:
            printf("PRINT\n");
            for (ASTList *a = node->print_stmt.args; a; a = a->next)
                ast_print(a->stmt, indent + 1);
            break;
        case AST_RETURN:
            printf("RETURN\n");
            ast_print(node->expr, indent + 1);
            break;
        case AST_BLOCK:
            printf("BLOCK\n");
            for (ASTList *s = node->block.stmts; s; s = s->next)
                ast_print(s->stmt, indent + 1);
            break;
        case AST_CAST:
            printf("CAST type=%d\n", node->cast.cast_type);
            ast_print(node->cast.operand, indent + 1);
            break;
        case AST_TERNARY:
            printf("TERNARY\n");
            ast_print(node->ternary.cond, indent + 1);
            ast_print(node->ternary.then_expr, indent + 1);
            ast_print(node->ternary.else_expr, indent + 1);
            break;
        case AST_EXPR_STMT:
            printf("EXPR_STMT\n");
            ast_print(node->expr, indent + 1);
            break;
        default:
            printf("NODE type=%d\n", node->type);
            break;
    }
}

void ast_print_program(ASTList *program) {
    int i = 1;
    for (ASTList *p = program; p; p = p->next, i++) {
        printf("  [stmt %d]\n", i);
        ast_print(p->stmt, 2);
    }
}

void ast_list_free(ASTList *list) {
    while (list) {
        ASTList *next = list->next;
        ast_free(list->stmt);
        free(list);
        list = next;
    }
}

void ast_free(ASTNode *node) {
    if (!node) return;
    switch (node->type) {
        case AST_VAR: free(node->var_name); break;
        case AST_NUMBER: break;
        case AST_ASSIGN:
            ast_free(node->assign.lhs);
            ast_free(node->assign.rhs);
            break;
        case AST_DECL:
            free(node->decl.var_name);
            ast_free(node->decl.init);
            break;
        case AST_BINARY_OP:
            ast_free(node->binop.left);
            ast_free(node->binop.right);
            break;
        case AST_UNARY_OP:
            ast_free(node->unop.operand);
            break;
        case AST_PRINT:
            ast_list_free(node->print_stmt.args);
            break;
        case AST_RETURN:
            ast_free(node->expr);
            break;
        case AST_IF:
            ast_free(node->if_stmt.condition);
            ast_free(node->if_stmt.then_stmt);
            ast_free(node->if_stmt.else_branch);
            break;
        case AST_WHILE:
            ast_free(node->while_stmt.condition);
            ast_free(node->while_stmt.do_stmt);
            break;
        case AST_FOR:
            ast_free(node->for_stmt.init);
            ast_free(node->for_stmt.condition);
            ast_free(node->for_stmt.update);
            ast_free(node->for_stmt.body);
            break;
        case AST_BLOCK:
            ast_list_free(node->block.stmts);
            break;
        default:
            break;
    }
    free(node);
}

ASTNode *parse_assignment_from_variable(ASTNode *lhs) {
    TokenType assign_type = current_token.type;
    advance();
    ASTNode *rhs = parse_expression();

    if (assign_type != TOKEN_ASSIGN) {
        int op;
        switch (assign_type) {
            case TOKEN_PLUS_ASSIGN: op = '+'; break;
            case TOKEN_MINUS_ASSIGN: op = '-'; break;
            case TOKEN_MUL_ASSIGN: op = '*'; break;
            case TOKEN_DIV_ASSIGN: op = '/'; break;
            case TOKEN_MOD_ASSIGN: op = '%'; break;
            case TOKEN_AND_ASSIGN: op = '&'; break;
            case TOKEN_OR_ASSIGN:  op = '|'; break;
            case TOKEN_XOR_ASSIGN: op = '^'; break;
            case TOKEN_SHL_ASSIGN: op = 'L'; break;
            case TOKEN_SHR_ASSIGN: op = 'R'; break;
            default:
                fprintf(stderr, "Unsupported compound assignment type: %d\n", assign_type);
                exit(EXIT_FAILURE);
        }

        ASTNode *load_lhs;
        if (lhs->type == AST_VAR) {
            load_lhs = new_node(AST_VAR);
            load_lhs->var_name = strdup(lhs->var_name);
        } else if (lhs->type == AST_ARRAY_ACCESS) {
            load_lhs = new_node(AST_ARRAY_ACCESS);
            load_lhs->array_access.array_name = strdup(lhs->array_access.array_name);
            load_lhs->array_access.index = lhs->array_access.index;
        } else {
            fprintf(stderr, "Invalid left-hand side in compound assignment.\n");
            exit(EXIT_FAILURE);
        }

        ASTNode *binop = new_node(AST_BINARY_OP);
        binop->binop.left = load_lhs;
        binop->binop.right = rhs;
        binop->binop.op = op;

        ASTNode *assign = new_node(AST_ASSIGN);
        assign->assign.lhs = lhs;
        assign->assign.rhs = binop;
        expect(TOKEN_SEMICOLON);
        return assign;
    }

    ASTNode *assign = new_node(AST_ASSIGN);
    assign->assign.lhs = lhs;
    assign->assign.rhs = rhs;
    expect(TOKEN_SEMICOLON);
    return assign;
}
