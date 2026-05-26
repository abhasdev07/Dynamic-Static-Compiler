#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_PRINT,
    TOKEN_RETURN,
    TOKEN_FLOAT_LITERAL,  // add this near TOKEN_NUMBER
    TOKEN_INT,
    TOKEN_FLOAT,
    TOKEN_CHAR,  // ✅ Added here
    TOKEN_STRING_LITERAL,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_CHAR_LITERAL,
    TOKEN_IDENTIFIER,
    TOKEN_ASSIGN,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LE,
    TOKEN_GE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_PERCENT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_BIT_NOT,
    TOKEN_LOG_NOT,
    TOKEN_BIT_AND,
    TOKEN_BIT_OR,
    TOKEN_BIT_XOR,
    TOKEN_SHL,
    TOKEN_SHR,
    TOKEN_SEMICOLON,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_QUESTION,   // ?
    TOKEN_COLON,    
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
    TOKEN_AND_ASSIGN,
    TOKEN_OR_ASSIGN,
    TOKEN_XOR_ASSIGN,
    TOKEN_SHL_ASSIGN,
    TOKEN_SHR_ASSIGN,
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_LBRACKET,   // [
    TOKEN_RBRACKET,   // ]
    TOKEN_COMMA,
    TOKEN_UNKNOWN
} TokenType;


typedef struct {
    TokenType type;
    int value;
    float float_value;  // ADD this line
    char *text;
} Token;

void lexer_init(FILE *source);
Token lexer_next_token(void);
const char *token_type_name(TokenType type);
void lexer_dump_all(FILE *source);

#endif
