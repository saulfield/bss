#ifndef BSS_H
#define BSS_H

#include <stdbool.h>

void assert(int condition, const char* message) {
    if (!condition) {
        printf("%s\n", message);
        exit(1);
    }
}

typedef enum ObjectType {
    TYPE_INT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_EMPTYLIST,
    TYPE_PAIR
} ObjectType;

typedef struct Object {
    ObjectType type;
    union {
        int int_val;
        bool bool_val;
        char* str_val;
        struct {
            struct Object* car;
            struct Object* cdr;
        };
    };
} Object;

typedef enum {
    TK_LPAREN = '(',
    TK_RPAREN = ')',
    TK_DOT = '.',
    TK_EOF = 128,
    TK_INT,
    TK_BOOL,
    TK_STRING,
    TK_SYMBOL,
} TokenKind;

typedef struct Token {
    TokenKind kind;
    union {
        int int_val;
        bool bool_val;
        char* str_val;
    };
} Token;

typedef struct LexState {
    FILE* stream;
    Token token;
} LexState;

#endif