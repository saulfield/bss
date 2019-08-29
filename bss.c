#define _GNU_SOURCE
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bss.h"

#define BUF_MAX 256

#define cadr(x) (car(cdr(x)))
#define cddr(x) (cdr(cdr(x)))

Object* empty_list;
Object* symbols_head;
Object* quote_symbol;
Object* true_obj;
Object* false_obj;

/* Object */

ObjectType type(Object* object) {
    return object->type;
}

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

Object* new_int(int val) {
    Object* object = new_object(TYPE_INT);
    object->int_val = val;
    return object;
}

Object* new_string(char* str) {
    Object* object = new_object(TYPE_STRING);
    object->str_val = str;
    return object;
}

Object* new_symbol(char* str) {
    // create object for the symbol
    Object* symbol = new_object(TYPE_SYMBOL);
    symbol->str_val = str;

    // add to head of symbol list
    Object* entry = cons(symbol, symbols_head);
    symbols_head = entry;
    return symbol;
}

Object* get_symbol(char* name) {
    Object* sym = symbols_head;
    while (sym != empty_list) {
        if (strncmp(car(sym)->str_val, name, strlen(name)) == 0) {
            return car(sym);
        }
        sym = cdr(sym);
    }
    return NULL;
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
            c = getc(ls->stream);
            assert(c == 't' || c == 'f', "bool must be #t or #f");
            ls->token.kind = TK_BOOL;
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
                symbol = new_symbol(str);
            }

            ls->token.sym_val = symbol;
            ls->token.kind = TK_SYMBOL;
        } break;

        case '\'':
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

        return cons(car_obj, cdr_obj);
    } else {
        // parse as a list
        Object* head = cons(car_obj, empty_list);
        Object* result = head;
        while (ls->token.kind != TK_RPAREN) {
            Object* tail = cons(parse_exp(ls), empty_list);
            head->cdr = tail;
            head = tail;
        }

        next_token(ls);
        return result;
    }
}

Object* parse_exp(LexState* ls) {
    Token token = ls->token;
    next_token(ls);

    switch (token.kind) {
        case TK_EOF: return NULL;
        case TK_INT: return new_int(token.int_val);
        case TK_BOOL: return token.bool_val ? true_obj : false_obj;
        case TK_SYMBOL: return token.sym_val;
        case TK_STRING: return new_string(token.str_val);
        case TK_LPAREN: return parse_pair(ls);
        case TK_QUOTE:
            return cons(quote_symbol, 
                        cons(parse_exp(ls),
                             empty_list));
        default:
            fprintf(stderr, "unexpected token: %d\n", ls->token.kind);
            exit(1);
    }
}

/* Eval */

Object* eval(Object* exp) {
    switch (type(exp)) {
        
        // self-evaluating
        case TYPE_BOOL:
        case TYPE_INT:
        case TYPE_STRING:
            return exp;

        case TYPE_PAIR:
            if (car(exp) == quote_symbol)
                return cadr(exp);

        default:
            fprintf(stderr, "unexpected type [%s]\n", type_names[type(exp)]);
            exit(1);
    }
}

/* Printing */

void print_object(Object* obj) {
    if (!obj) return;

    switch(type(obj)) {
        case TYPE_INT: printf("%d", obj->int_val); break;
        case TYPE_BOOL:  printf("%s", obj->bool_val ? "#t" : "#f"); break;
        case TYPE_STRING: printf("\"%s\"", obj->str_val); break;
        case TYPE_SYMBOL: printf("%s", obj->str_val); break;
        case TYPE_EMPTYLIST: printf("()"); break;
        case TYPE_PAIR:
            printf("(");

            Object* o = obj;
            while (type(cdr(o)) == TYPE_PAIR && cdr(o) != empty_list) {
                print_object(car(o));
                printf(" ");
                o = cdr(o);
            }

            if (cdr(o) == empty_list) {
                print_object(car(o));
            } else {
                print_object(car(o));
                printf(" . ");
                print_object(cdr(o));
            }
            printf(")");
            break;
        default:
            fprintf(stderr, "unexpected type [%s]\n", type_names[type(obj)]);
            exit(1);
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
    empty_list = new_object(TYPE_EMPTYLIST);

    true_obj = new_object(TYPE_BOOL);
    true_obj->bool_val = true;
    false_obj = new_object(TYPE_BOOL);
    false_obj->bool_val = false;

    symbols_head = empty_list;
    quote_symbol = new_symbol("quote");
}

void eval_all(LexState* ls) {
    next_token(ls);
    while (ls->token.kind != TK_EOF) {
        Object* exp = parse_exp(ls);
        Object* result = eval(exp);
        if (result) {
            print_object(result);
            printf("\n");
        }
    }
}

int main(int argc, char** argv) {
    LexState ls = {};

    init();

    if (argc == 3 && !strncmp(argv[1], "-f", 2)) {
        FILE* file = fopen(argv[2], "r");
        if (file == NULL) {
            fprintf(stderr, "could not open file: %s\n", argv[2]);
            exit(1);
        }

        ls.stream = file;
        eval_all(&ls);
        fclose(file);
    } else {
        printf("Welcome to Bootstrap Scheme\n");
        
        while (true) {
            char buf[BUF_MAX];
            size_t len = 0;
            
            printf("> ");
            int c = getc(stdin);
            while (c != '\n') {
                buf[len++] = c;
                c = getc(stdin);
            }
            buf[len++] = '\n';

            FILE* stream = fmemopen(buf, len, "r");
            ls.stream = stream;
            eval_all(&ls);
            fclose(stream);
        }
    }

    return 0;
}
