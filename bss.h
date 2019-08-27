#ifndef BSS_H
#define BSS_H

// #include <stdbool.h>

void assert(int condition, const char* message) {
    if (!condition) {
        printf("%s\n", message);
        exit(1);
    }
}

typedef enum ObjectType {
    TYPE_INT,
    TYPE_BOOL,
} ObjectType;

typedef struct Object {
    ObjectType type;
    union {
        int int_val;
        bool bool_val;
    };
} Object;

typedef enum {
    TK_EOF,
    TK_INT,
    TK_SYMBOL,
    TK_BOOL,
    TK_LPAREN,
    TK_RPAREN,
} TokenKind;

typedef struct Token {
    TokenKind kind;
    union {
        int int_val;
        bool bool_val;
    };
} Token;

#endif