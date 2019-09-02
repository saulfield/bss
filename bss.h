#ifndef BSS_H
#define BSS_H

#include <stdbool.h>

void assert(int condition, const char* message) {
    if (!condition) {
        printf("%s\n", message);
        exit(1);
    }
}

const bool valid_chars[] = {
    ['+'] = true,
    ['-'] = true,
    ['/'] = true,
    ['*'] = true,
    ['='] = true,
    ['_'] = true,
    ['!'] = true,
    ['?'] = true,
};

typedef enum ObjectType {
    TYPE_INT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_SYMBOL,
    TYPE_EMPTYLIST,
    TYPE_PAIR,
    TYPE_PRIMITIVE,
    TYPE_PROCEDURE
} ObjectType;

const char* type_names[] = {
    [TYPE_INT] = "TYPE_INT",
    [TYPE_BOOL] = "TYPE_BOOL",
    [TYPE_STRING] = "TYPE_STRING",
    [TYPE_SYMBOL] = "TYPE_SYMBOL",
    [TYPE_EMPTYLIST] = "TYPE_EMPTYLIST",
    [TYPE_PAIR] = "TYPE_PAIR",
    [TYPE_PRIMITIVE] = "TYPE_PRIMITIVE",
    [TYPE_PROCEDURE] = "TYPE_PROCEDURE",
};

typedef struct Object {
    ObjectType type;
    union {
        int int_val;
        bool bool_val;
        char* str_val;
        struct Object* (*func)(struct Object* args);
        struct {
            struct Object* params;
            struct Object* body;
            struct Object* env;
        };
        struct {
            struct Object* car;
            struct Object* cdr;
        };
    };
} Object;

typedef enum {
    TK_NONE = 0,
    TK_QUOTE = '\'',
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
        Object* sym_val;
    };
} Token;

typedef struct LexState {
    FILE* stream;
    Token token;
} LexState;

Object* parse_exp(LexState* ls);
void print_object(Object* obj);
Object* eval(Object* exp, Object* env);
Object* apply(Object* proc, Object* args);
void eval_all(LexState* ls, bool verbose);

#endif