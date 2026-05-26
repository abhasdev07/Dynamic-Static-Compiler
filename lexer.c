#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"

int current_line = 1;
static FILE *input;

void lexer_init(FILE *source) {
    input = source;

    // Skip BOM if present
    int c1 = fgetc(input);
    int c2 = fgetc(input);
    int c3 = fgetc(input);
    if (!(c1 == 0xEF && c2 == 0xBB && c3 == 0xBF)) {
        if (c3 != EOF) ungetc(c3, input);
        if (c2 != EOF) ungetc(c2, input);
        if (c1 != EOF) ungetc(c1, input);
    }
}

static void skip_whitespace() {
    int c;
    while ((c = fgetc(input)) != EOF && isspace(c)) {
        if (c == '\n') current_line++;
        //printf("[Lexer Debug] Skipping char: '%c' (%d)\n", c, c);
    }
    if (c != EOF) ungetc(c, input);
}

Token lexer_next_token() {
    skip_whitespace();
    Token tok = {TOKEN_UNKNOWN, 0, 0 ,  NULL};
    int c = fgetc(input);
    //printf("[Lexer Debug] Read char: '%c' (%d)\n", c, c);

    if (c == EOF) {
        tok.type = TOKEN_EOF;
        //printf("[Lexer] Line %d: Token %d\n", current_line, tok.type);
        return tok;
    }

    // Number parsing
    
    if (c == '\'') {
        int ch = fgetc(input);
        fgetc(input); // consume closing quote '
        tok.type = TOKEN_CHAR_LITERAL;
        tok.value = ch;
        //printf("[Lexer] Line %d: Token %d (%d)\n", current_line, tok.type, tok.value);
        return tok;
    }

    if (isdigit(c) || c == '.') {
        char buf[64];
        int i = 0;
        int is_float = 0;

        if (c == '.') {
            is_float = 1;
            buf[i++] = c;
            c = fgetc(input);
        }

        while (isdigit(c)) {
            buf[i++] = c;
            c = fgetc(input);
        }

        if (c == '.') {
            is_float = 1;
            buf[i++] = c;
            c = fgetc(input);
            while (isdigit(c)) {
                buf[i++] = c;
                c = fgetc(input);
            }
        }

        buf[i] = '\0';
        if (c != EOF) ungetc(c, input);

        if (is_float) {
            tok.type = TOKEN_FLOAT_LITERAL;
            tok.float_value = atof(buf);
        } else {
            tok.type = TOKEN_NUMBER;
            tok.value = atoi(buf);
        }
        return tok;
    }

    if (c == '"') {
        char buf[256];
        int i = 0;
        while ((c = fgetc(input)) != '"' && c != EOF) {
            buf[i++] = c;
        }
        buf[i] = '\0';
        tok.type = TOKEN_STRING_LITERAL;
        tok.text = strdup(buf);
        return tok;
    }

    // Identifier / keyword
    if (isalpha(c) || c == '_') {
        char buf[256];
        int len = 0;
        buf[len++] = c;
        while (isalnum(c = fgetc(input)) || c == '_') {
            buf[len++] = c;
        }
        if (c != EOF) ungetc(c, input);
        buf[len] = '\0';
        if (strcmp(buf, "int") == 0)
            tok.type = TOKEN_INT;
        else if (strcmp(buf, "char") == 0)
            tok.type = TOKEN_CHAR;
        else if (strcmp(buf, "string") == 0)
            tok.type = TOKEN_STRING;  
        else if (strcmp(buf, "float") == 0)
            tok.type = TOKEN_FLOAT;
        else if (strcmp(buf, "if") == 0)
            tok.type = TOKEN_IF;
        else if (strcmp(buf, "else") == 0)
            tok.type = TOKEN_ELSE;
        else if (strcmp(buf, "while") == 0)
            tok.type = TOKEN_WHILE;
        else if (strcmp(buf, "for") == 0)
            tok.type = TOKEN_FOR;
        else if (strcmp(buf, "print") == 0)
            tok.type = TOKEN_PRINT;
        else if (strcmp(buf, "return") == 0)
            tok.type = TOKEN_RETURN;
        else {
            tok.type = TOKEN_IDENTIFIER;
            tok.text = strdup(buf);
        }
        //printf("[Lexer] Line %d: Token %d\n", current_line, tok.type);
        return tok;

    }    

    int next;
    switch (c) {
        case '+':
            next = fgetc(input);
            if (next == '+') tok.type = TOKEN_INC;
            else if (next == '=') tok.type = TOKEN_PLUS_ASSIGN;
            else { tok.type = TOKEN_PLUS; ungetc(next, input); }
            break;
        case '-':
            next = fgetc(input);
            if (next == '-') tok.type = TOKEN_DEC;
            else if (next == '=') tok.type = TOKEN_MINUS_ASSIGN;
            else { tok.type = TOKEN_MINUS; ungetc(next, input); }
            break;
        case '*':
            next = fgetc(input);
            if (next == '=') tok.type = TOKEN_MUL_ASSIGN;
            else { tok.type = TOKEN_STAR; ungetc(next, input); }
            break;
        case '/':
            next = fgetc(input);
            if (next == '=') tok.type = TOKEN_DIV_ASSIGN;
            else { tok.type = TOKEN_SLASH; ungetc(next, input); }
            break;
        case '%':
            next = fgetc(input);
            if (next == '=') tok.type = TOKEN_MOD_ASSIGN;
            else { tok.type = TOKEN_PERCENT; ungetc(next, input); }
            break;
        case '&':
            next = fgetc(input);
            if (next == '&') tok.type = TOKEN_AND;
            else if (next == '=') tok.type = TOKEN_AND_ASSIGN;
            else { tok.type = TOKEN_BIT_AND; ungetc(next, input); }
            break;
        case '|':
            next = fgetc(input);
            if (next == '|') tok.type = TOKEN_OR;
            else if (next == '=') tok.type = TOKEN_OR_ASSIGN;
            else { tok.type = TOKEN_BIT_OR; ungetc(next, input); }
            break;
        case '^':
            next = fgetc(input);
            if (next == '=') tok.type = TOKEN_XOR_ASSIGN;
            else { tok.type = TOKEN_BIT_XOR; ungetc(next, input); }
            break;
        case '<':
            next = fgetc(input);
            if (next == '<') {
                int next2 = fgetc(input);
                if (next2 == '=') tok.type = TOKEN_SHL_ASSIGN;
                else { tok.type = TOKEN_SHL; ungetc(next2, input); }
            } else if (next == '=') tok.type = TOKEN_LE;
            else { tok.type = TOKEN_LT; ungetc(next, input); }
            break;
        case '>':
            next = fgetc(input);
            if (next == '>') {
                int next2 = fgetc(input);
                if (next2 == '=') tok.type = TOKEN_SHR_ASSIGN;
                else { tok.type = TOKEN_SHR; ungetc(next2, input); }
            } else if (next == '=') tok.type = TOKEN_GE;
            else { tok.type = TOKEN_GT; ungetc(next, input); }
            break;
        case '=':
            next = fgetc(input);
            tok.type = (next == '=') ? TOKEN_EQ : TOKEN_ASSIGN;
            if (next != '=') ungetc(next, input);
            break;
        case '!':
            next = fgetc(input);
            if (next == '=') tok.type = TOKEN_NEQ;
            else { tok.type = TOKEN_LOG_NOT; ungetc(next, input); }
            break;
        case ',': tok.type = TOKEN_COMMA; break;
        case '~': tok.type = TOKEN_BIT_NOT; break;
        case '(': tok.type = TOKEN_LPAREN; break;
        case ')': tok.type = TOKEN_RPAREN; break;
        case '{': tok.type = TOKEN_LBRACE; break;
        case '}': tok.type = TOKEN_RBRACE; break;
        case ';': tok.type = TOKEN_SEMICOLON; break;
        case '?': tok.type = TOKEN_QUESTION; break;
        case ':': tok.type = TOKEN_COLON; break;
        case '[': tok.type = TOKEN_LBRACKET; break;
        case ']': tok.type = TOKEN_RBRACKET; break;

        default:
            fprintf(stderr, "Unknown character: '%c' (%d)\n", c, c);
            tok.type = TOKEN_UNKNOWN;
            break;
    }

    //printf("[Lexer] Line %d: Token %d\n", current_line, tok.type);
    return tok;
}

const char *token_type_name(TokenType type) {
    switch (type) {
        case TOKEN_EOF: return "EOF";
        case TOKEN_IF: return "IF";
        case TOKEN_ELSE: return "ELSE";
        case TOKEN_WHILE: return "WHILE";
        case TOKEN_FOR: return "FOR";
        case TOKEN_PRINT: return "PRINT";
        case TOKEN_RETURN: return "RETURN";
        case TOKEN_FLOAT_LITERAL: return "FLOAT_LITERAL";
        case TOKEN_INT: return "INT";
        case TOKEN_FLOAT: return "FLOAT";
        case TOKEN_CHAR: return "CHAR";
        case TOKEN_STRING_LITERAL: return "STRING_LITERAL";
        case TOKEN_STRING: return "STRING";
        case TOKEN_NUMBER: return "NUMBER";
        case TOKEN_CHAR_LITERAL: return "CHAR_LITERAL";
        case TOKEN_IDENTIFIER: return "IDENTIFIER";
        case TOKEN_ASSIGN: return "ASSIGN";
        case TOKEN_EQ: return "EQ";
        case TOKEN_NEQ: return "NEQ";
        case TOKEN_LT: return "LT";
        case TOKEN_GT: return "GT";
        case TOKEN_LE: return "LE";
        case TOKEN_GE: return "GE";
        case TOKEN_AND: return "AND";
        case TOKEN_OR: return "OR";
        case TOKEN_PLUS: return "PLUS";
        case TOKEN_MINUS: return "MINUS";
        case TOKEN_STAR: return "STAR";
        case TOKEN_SLASH: return "SLASH";
        case TOKEN_PERCENT: return "PERCENT";
        case TOKEN_LPAREN: return "LPAREN";
        case TOKEN_RPAREN: return "RPAREN";
        case TOKEN_BIT_NOT: return "BIT_NOT";
        case TOKEN_LOG_NOT: return "LOG_NOT";
        case TOKEN_BIT_AND: return "BIT_AND";
        case TOKEN_BIT_OR: return "BIT_OR";
        case TOKEN_BIT_XOR: return "BIT_XOR";
        case TOKEN_SHL: return "SHL";
        case TOKEN_SHR: return "SHR";
        case TOKEN_SEMICOLON: return "SEMICOLON";
        case TOKEN_LBRACE: return "LBRACE";
        case TOKEN_RBRACE: return "RBRACE";
        case TOKEN_QUESTION: return "QUESTION";
        case TOKEN_COLON: return "COLON";
        case TOKEN_PLUS_ASSIGN: return "PLUS_ASSIGN";
        case TOKEN_MINUS_ASSIGN: return "MINUS_ASSIGN";
        case TOKEN_MUL_ASSIGN: return "MUL_ASSIGN";
        case TOKEN_DIV_ASSIGN: return "DIV_ASSIGN";
        case TOKEN_MOD_ASSIGN: return "MOD_ASSIGN";
        case TOKEN_AND_ASSIGN: return "AND_ASSIGN";
        case TOKEN_OR_ASSIGN: return "OR_ASSIGN";
        case TOKEN_XOR_ASSIGN: return "XOR_ASSIGN";
        case TOKEN_SHL_ASSIGN: return "SHL_ASSIGN";
        case TOKEN_SHR_ASSIGN: return "SHR_ASSIGN";
        case TOKEN_INC: return "INC";
        case TOKEN_DEC: return "DEC";
        case TOKEN_LBRACKET: return "LBRACKET";
        case TOKEN_RBRACKET: return "RBRACKET";
        case TOKEN_COMMA: return "COMMA";
        default: return "UNKNOWN";
    }
}

void lexer_dump_all(FILE *source) {
    fseek(source, 0, SEEK_SET);
    lexer_init(source);
    int n = 1;
    for (;;) {
        Token tok = lexer_next_token();
        printf("  %4d  %-18s", n++, token_type_name(tok.type));
        if (tok.type == TOKEN_NUMBER || tok.type == TOKEN_CHAR_LITERAL)
            printf("  value=%d", tok.value);
        else if (tok.type == TOKEN_FLOAT_LITERAL)
            printf("  value=%f", tok.float_value);
        else if (tok.type == TOKEN_IDENTIFIER || tok.type == TOKEN_STRING_LITERAL)
            printf("  text=\"%s\"", tok.text ? tok.text : "");
        printf("\n");
        if (tok.type == TOKEN_EOF) break;
    }
    fseek(source, 0, SEEK_SET);
}
