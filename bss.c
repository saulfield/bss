#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bss.h"

#define BUF_MAX 256

static Token token;
Object* true_obj;
Object* false_obj;
Object* empty_list;

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
    int c = getc(stream);
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
    int sign = 1;
    char buf[BUF_MAX];

    switch (c) {
        case EOF:
            token.kind = TK_EOF;
            break;

        case '#':
            token.kind = TK_BOOL;
            c = getc(stream);
            assert(c == 't' || c == 'f', "bool must be #t or #f");
            token.bool_val = c == 't' ? true : false;
            break;

        case '-':
            c = getc(stream);
            sign = -1;
        case '0'...'9':
            ungetc(c, stream);
            int value = read_int(stream);
            token.kind = TK_INT;
            token.int_val = sign * value;
            break;

        case '\"':
            c = getc(stream);
            int len = 0;
            while (c != EOF && c != '\"') {
                buf[len++] = c;
                c = getc(stream);
            }
            char* str = malloc(len + 1);
            memcpy(str, buf, len);
            str[len+1] = '\0';
            token.kind = TK_STRING;
            token.str_val = str;
            break;

        case '(':
        case ')':
            token.kind = c;
            break;

        default:
            fprintf(stderr, "unexpected character: %c\n", c);
            exit(1);
    }
}

/* Parse */

Object* parse(FILE* stream) {
    next_token(stream);
    
    switch (token.kind) {
        case TK_EOF:
            return NULL;

        case TK_BOOL:
            if (token.bool_val)
                return true_obj;
            return false_obj;

        case TK_INT: {
            Object* object = new_object(TYPE_INT);
            object->int_val = token.int_val;
            return object;
        }

        case TK_STRING: {
            Object* object = new_object(TYPE_STRING);
            object->str_val = token.str_val;
            return object;
        }

        case TK_LPAREN:
            next_token(stream);
            assert(token.kind == TK_RPAREN, "expected )");
            return empty_list;

        default:
            fprintf(stderr, "unexpected token: %d\n", token.kind);
            exit(1);
    }
}

/* Printing */

void print_object(Object* object) {
    if (!object) return;

    switch(object->type) {
        case TYPE_INT:
            printf("%d", object->int_val);
            break;
        case TYPE_BOOL:
            if (object->bool_val)
                printf("#t");
            else
                printf("#f");
            break;
        case TYPE_STRING:
            printf("\"%s\"", object->str_val);
            break;
        case TYPE_EMPTYLIST:
            printf("()");
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

void init() {
    true_obj = new_object(TYPE_BOOL);
    true_obj->bool_val = true;

    false_obj = new_object(TYPE_BOOL);
    false_obj->bool_val = false;

    empty_list = new_object(TYPE_EMPTYLIST);
}

int main(int argc, char** argv) {
    init();
    
    if (argc == 1) {
        printf("Welcome to Bootstrap Scheme\n");
        while (true) {
            printf("> ");
            while (peek(stdin) != '\n') {
                print_object(parse(stdin));
                printf("\n");
                skip_repl_space(stdin);
            }
            getc(stdin);
        }
    } else if (!strncmp(argv[1], "-f", 2)) {
        FILE* file = fopen("input.scm", "r");
        while (peek(file) != EOF) {
            Object* object = parse(file);
            if (object) {
                print_object(object);
                printf("\n");
            }
        }
    }

    return 0;
}
