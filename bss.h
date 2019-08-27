#ifndef BSS_H
#define BSS_H

typedef enum ObjectType {
    TYPE_INT
} ObjectType;

typedef struct Object {
    ObjectType type;
    union {
        int int_val;
    };
} Object;

typedef enum {
    TK_INT,
    TK_SYMBOL,
    TK_LPAREN,
    TK_RPAREN,
} TokenKind;

typedef struct Token {
    TokenKind kind;
    union {
        int int_val;
    };
} Token;

#endif