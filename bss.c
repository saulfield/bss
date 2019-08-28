#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bss.h"

#define BUF_MAX 256

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

void next_token(LexState* ls) {
    skip_whitespace(ls->stream);
    int c = getc(ls->stream);
    int sign = 1;
    char buf[BUF_MAX];

    switch (c) {
        case EOF:
            ls->token.kind = TK_EOF;
            break;

        case '#':
            ls->token.kind = TK_BOOL;
            c = getc(ls->stream);
            assert(c == 't' || c == 'f', "bool must be #t or #f");
            ls->token.bool_val = c == 't' ? true : false;
            break;

        case '-':
            c = getc(ls->stream);
            sign = -1;
        case '0'...'9':
            ungetc(c, ls->stream);
            int value = read_int(ls->stream);
            ls->token.kind = TK_INT;
            ls->token.int_val = sign * value;
            break;

        case '\"':
            c = getc(ls->stream);
            int len = 0;
            while (c != EOF && c != '\"') {
                buf[len++] = c;
                c = getc(ls->stream);
            }
            char* str = malloc(len + 1);
            memcpy(str, buf, len);
            str[len+1] = '\0';
            ls->token.kind = TK_STRING;
            ls->token.str_val = str;
            break;

        case '(':
        case ')':
        case '.':
            ls->token.kind = c;
            break;

        default:
            fprintf(stderr, "unexpected character: %c\n", c);
            exit(1);
    }
}

/* Parse */

Object* parse(LexState* ls) {
    next_token(ls);
    
    switch (ls->token.kind) {
        case TK_EOF:
            return NULL;

        case TK_BOOL:
            if (ls->token.bool_val)
                return true_obj;
            return false_obj;

        case TK_INT: {
            Object* object = new_object(TYPE_INT);
            object->int_val = ls->token.int_val;
            return object;
        }

        case TK_STRING: {
            Object* object = new_object(TYPE_STRING);
            object->str_val = ls->token.str_val;
            return object;
        }

        case TK_LPAREN:
            next_token(ls);
            assert(ls->token.kind == TK_RPAREN, "expected )");
            return empty_list;

        default:
            fprintf(stderr, "unexpected token: %d\n", ls->token.kind);
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
    LexState ls = {};
    init();

    if (argc == 1) {
        printf("Welcome to Bootstrap Scheme\n");

        ls.stream = stdin;
        while (true) {
            printf("> ");
            while (peek(stdin) != '\n') {
                print_object(parse(&ls));
                printf("\n");
                skip_repl_space(stdin);
            }
            getc(stdin);
        }
    } else if (!strncmp(argv[1], "-f", 2)) {
        FILE* file = fopen(argv[2], "r");
        if (file == NULL) {
            fprintf(stderr, "could not open file: %s\n", argv[2]);
            exit(1);
        }

        ls.stream = file;
        while (peek(file) != EOF) {
            Object* object = parse(&ls);
            if (object) {
                print_object(object);
                printf("\n");
            }
        }
    }

    return 0;
}
