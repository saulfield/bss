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
Object* symbols_head;

/* Object */

Object* new_object(ObjectType type) {
    Object* object = malloc(sizeof(Object));
    object->type = type;
    return object;
}

Object* cons(Object* car, Object* cdr) {
    Object* object = new_object(TYPE_PAIR);
    object->car = car;
    object->cdr = cdr;
    return object;
}

Object* car(Object* pair) {
    return pair->car;
}

Object* cdr(Object* pair) {
    return pair->cdr;
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
            while (c != EOF && c != '\n') {
                c = getc(stream);
            }
            continue;
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

Object* get_symbol(char* name) {
    Object* sym = symbols_head;
    while (sym != empty_list) {
        if (strcmp(sym->str_val, name) == 0) {
            return sym;
        }
        sym = sym->cdr;
    }
    return NULL;
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
            str[len] = '\0';

            ls->token.kind = TK_STRING;
            ls->token.str_val = str;
            break;

        case '_':
        case 'A'...'Z':
        case 'a'...'z': {
            int len = 0;
            while (isalnum(c) || c == '_') {
                buf[len++] = c;
                c = getc(ls->stream);
            }
            buf[len] = '\0';

            Object* symbol = get_symbol(buf);
            if (symbol == NULL) {
                // allocate memory for the string
                char* str = malloc(len + 1);
                memcpy(str, buf, len);
                str[len] = '\0';

                // create object for the symbol
                symbol = new_object(TYPE_SYMBOL);
                symbol->str_val = str;

                // add to head of symbol list
                Object* entry = cons(symbol, symbols_head);
                symbols_head = entry;
            }

            ls->token.sym_val = symbol;
            ls->token.kind = TK_SYMBOL;
        } break;

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

Object* parse_pair(LexState* ls) {
    if (ls->token.kind == TK_RPAREN) {
        next_token(ls);
        return empty_list;
    }

    Object* car_obj = parse_exp(ls);

    if (ls->token.kind == TK_DOT) {
        // parse as a pair
        next_token(ls);
        Object* cdr_obj = parse_exp(ls);
        assert(ls->token.kind == TK_RPAREN, "expected )");
        next_token(ls);

        Object* result = cons(car_obj, cdr_obj);
        return result;
    } else {
        // parse as a list
        Object* cadr_obj = parse_exp(ls);
        assert(ls->token.kind == TK_RPAREN, "expected )");
        next_token(ls);

        Object* cdr_obj = cons(cadr_obj, empty_list);
        Object* result = cons(car_obj, cdr_obj);
        return result;
    }
}

Object* parse_exp(LexState* ls) {
    if (ls->token.kind == TK_NONE)
        next_token(ls);

    Token token = ls->token;
    next_token(ls);

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

        case TK_SYMBOL:
            return token.sym_val;

        case TK_LPAREN: {
            Object* pair = parse_pair(ls);
            return pair;
        }

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
        case TYPE_SYMBOL:
            printf("%s", object->str_val);
            break;
        case TYPE_EMPTYLIST:
            printf("()");
            break;
        case TYPE_PAIR:
            printf("(");

            Object* o = object;
            while (o->cdr->type == TYPE_PAIR && o->cdr != empty_list) {
                print_object(o->car);
                printf(" ");
                o = o->cdr;
            }

            if (o->cdr == empty_list) {
                print_object(o->car);
            } else {
                print_object(o->car);
                printf(" . ");
                print_object(o->cdr);
            }
            printf(")");
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

    symbols_head = empty_list;
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
                print_object(parse_exp(&ls));
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
        next_token(&ls);
        while (peek(file) != EOF) {
            Object* object = parse_exp(&ls);
            if (object) {
                print_object(object);
                printf("\n");
            }
        }
    }

    return 0;
}
