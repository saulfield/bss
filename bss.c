#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bss.h"

static Token token;

/* Object */

Object* new_object(ObjectType type) {
    Object* object = malloc(sizeof(Object));
    object->type = type;
    return object;
}

/* Lex */

int peek(FILE* stream) {
    int c = getc(stream);
    ungetc(c, stream);
    return c;
}

void skip_whitespace(FILE* stream) {
    int c = getc(stream);
    while (c != EOF) {
        if (isspace(c)) {
            c = getc(stream);
            continue;
        }

        if (c == ';') {
            while (c != EOF || c != '\n') {
                c = getc(stream);
            }
        }
        ungetc(c, stream);
        break;
    }
}

int read_int(FILE* stream) {
    char c = getc(stream);
    int value = c - '0';

    c = getc(stream);
    while (isdigit(c)) {
        value *= 10;
        value += c - '0';
        c = getc(stream);
    }
    ungetc(c, stream);
    return value;
}

void next_token(FILE* stream) {
    skip_whitespace(stream);
    int c = getc(stream);
    if (c == EOF) return;

    if (c == '-') {
        int value = read_int(stream);
        token.kind = TK_INT;
        token.int_val = -value;
    } else if (isdigit(c)) {
        ungetc(c, stream);
        int value = read_int(stream);
        token.kind = TK_INT;
        token.int_val = value;
    } else {
        fprintf(stderr, "unexpected character: %c\n", c);
        exit(1);
    }
}

/* Parse */

Object* parse(FILE* stream) {
    next_token(stream);

    if (token.kind == TK_INT) {
        Object* object = new_object(TYPE_INT);
        object->int_val = token.int_val;
        return object;
    } else {
        fprintf(stderr, "unexpected token: %d\n", token.kind);
        exit(1);
    }
}

/* Printing */

void print_object(Object* object) {
    switch(object->type) {
        case TYPE_INT:
            printf("%d", object->int_val);
            break;
        default:
            printf("TYPE_UNKNOWN [%d]", object->type);
            break;
    }
}

/* Main */

void skip_repl_space(FILE* stream) {
    int c = getc(stream);
    while (c == ' ') {
        c = getc(stream);
    }
    ungetc(c, stream);
}

int main(void) {
    // FILE* file = fopen("input.scm", "r");
    
    // while (peek(file) != '\n') {
    //     next_token(file);
    //     printf("%d\n", token.int_value);
    //     skip_repl_space(file);
    // }

    /* REPL */
    printf("Welcome to Bootstrap Scheme v0.1\n");
    while (true) {
        printf("> ");
        while (peek(stdin) != '\n') {
            print_object(parse(stdin));
            printf("\n");
            skip_repl_space(stdin);
        }
        getc(stdin);
    }

    return 0;
}
