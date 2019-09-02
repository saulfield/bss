#define _GNU_SOURCE
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bss.h"

#define BUF_MAX 256

#define caar(x) (car(car(x)))
#define cadr(x) (car(cdr(x)))
#define cddr(x) (cdr(cdr(x)))
#define cadar(x) (car(cdr(car(x))))
#define caddr(x) (car(cddr(x)))
#define cdddr(x) (cdr(cddr(x)))
#define cadddr(x) (car(cdddr(x)))

Object* empty_list;
Object* global_env;
Object* true_obj;
Object* false_obj;

Object* symbols_head;
Object* quote_symbol;
Object* define_symbol;
Object* set_symbol;
Object* ok_symbol;
Object* if_symbol;
Object* lambda_symbol;
Object* cond_symbol;
Object* else_symbol;
Object* apply_symbol;

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
        char* entry = car(sym)->str_val;
        if (strcmp(entry, name) == 0) {
            return car(sym);
        }
        sym = cdr(sym);
    }
    return NULL;
}

Object* new_primitive(Object* (*func)(Object*)) {
    Object* primitive = new_object(TYPE_PRIMITIVE);
    primitive->func = func;
    return primitive;
}

Object* new_procedure(Object* params, Object* body, Object* env) {
    Object* proc = new_object(TYPE_PROCEDURE);
    proc->params = params;
    proc->body = body;
    proc->env = env;
    return proc;
}

Object* _proc_add(Object* args) {
    int result = 0;

    while (args != empty_list) {
        Object* obj = car(args);
        assert(type(obj) == TYPE_INT, "expected TYPE_INT");

        result += obj->int_val;
        args = cdr(args);
    }
    return new_int(result);
}

Object* _proc_sub(Object* args) {
    Object* obj = car(args);
    int result = obj->int_val;
    args = cdr(args);

    while (args != empty_list) {
        obj = car(args);
        assert(type(obj) == TYPE_INT, "expected TYPE_INT");

        result -= obj->int_val;
        args = cdr(args);
    }
    return new_int(result);
}

Object* _proc_mul(Object* args) {
    int result = 1;

    while (args != empty_list) {
        Object* obj = car(args);
        assert(type(obj) == TYPE_INT, "expected TYPE_INT");

        result *= obj->int_val;
        args = cdr(args);
    }

    return new_int(result);
}

Object* _proc_div(Object* args) {
    Object* obj = car(args);
    assert(type(obj) == TYPE_INT, "expected TYPE_INT");

    int result = obj->int_val;
    args = cdr(args);

    while (args != empty_list) {
        obj = car(args);
        assert(type(obj) == TYPE_INT, "expected TYPE_INT");

        result /= obj->int_val;
        args = cdr(args);
    }
    return new_int(result);
}

Object* _proc_equals(Object* args) {
    Object* obj = car(args);
    assert(type(obj) == TYPE_INT, "expected TYPE_INT");

    int initial_val = obj->int_val;
    args = cdr(args);

    while (args != empty_list) {
        obj = car(args);
        assert(type(obj) == TYPE_INT, "expected TYPE_INT");

        if (obj->int_val != initial_val)
            return false_obj;

        args = cdr(args);
    }
    return true_obj;
}

Object* bool_object(bool expression) {
    return expression ? true_obj : false_obj;
}

Object* _proc_not(Object* args) {
    Object* arg = car(args);
    assert(type(arg) == TYPE_BOOL, "expected bool");
    return bool_object(!arg->bool_val);
}

Object* _proc_is_null(Object* args) {
    return bool_object(car(args) == empty_list);
}

Object* _proc_is_eq(Object* args) {
    Object* a = car(args);
    Object* b = cadr(args);

    if (type(a) != type(b))
        return false_obj;

    switch(type(a)) {
        case TYPE_INT: 
            return bool_object(a->bool_val == b->bool_val);
        case TYPE_STRING: 
            return bool_object(!strcmp(a->str_val, b->str_val));
        default: 
            return bool_object(a == b);
    }
}

Object* _proc_is_number(Object* args) {
    return bool_object(type(car(args)) == TYPE_INT);
}

Object* _proc_is_string(Object* args) {
    return bool_object(type(car(args)) == TYPE_STRING);
}

Object* _proc_is_symbol(Object* args) {
    return bool_object(type(car(args)) == TYPE_SYMBOL);
}

Object* _proc_is_pair(Object* args) {
    return bool_object(type(car(args)) == TYPE_PAIR);
}

Object* _proc_list(Object* args) {
    if (args == empty_list)
        return empty_list;

    return cons(car(args), _proc_list(cdr(args)));
}

Object* _proc_car(Object* args) {
    return car(car(args));
}

Object* _proc_cdr(Object* args) {
    return cdr(car(args));
}

Object* _proc_cons(Object* args) {
    return cons(car(args), cadr(args));
}

Object* _proc_set_car(Object* args) {
    Object* pair = car(args);
    pair->car = cadr(args);
    return ok_symbol;
}

Object* _proc_set_cdr(Object* args) {
    Object* pair = car(args);
    pair->cdr = cadr(args);
    return ok_symbol;
}

/* Environment */

Object* extend_environment(Object* vars, Object* vals, Object* env) {
    return cons(cons(vars, vals), env);
}

void add_binding(Object* var, Object* val, Object* frame) {
    frame->car = cons(var, car(frame));
    frame->cdr = cons(val, cdr(frame));
}

void define_variable(Object* var, Object* val, Object* env) {
    Object* frame = car(env);
    Object* vars = car(frame);
    Object* vals = cdr(frame);

    while (vars != empty_list) {
        if (car(vars) == var) {
            vals->car = val;
            return;
        }
        vars = cdr(vars);
        vals = cdr(vals);
    }
    add_binding(var, val, frame);
}

void set_variable_value(Object* var, Object* val, Object* env) {
    Object* frame;
    Object* vars;
    Object* vals;

    while (env != empty_list) {
        frame = car(env);
        vars = car(frame);
        vals = cdr(frame);

        while (vars != empty_list) {
            if (car(vars) == var) {
                vals->car = val;
                return;
            }
            vars = cdr(vars);
            vals = cdr(vals);
        }
        env = cdr(env);
    }

    fprintf(stderr, "unbound variable: %s\n", var->str_val);
    exit(1);
}

Object* lookup_variable(Object* var, Object* env) {
    Object* frame;
    Object* vars;
    Object* vals;

    while (env != empty_list) {
        frame = car(env);
        vars = car(frame);
        vals = cdr(frame);

        while (vars != empty_list) {
            Object* sym = car(vars);
            if (sym == var) {
                return car(vals);
            }
            vars = cdr(vars);
            vals = cdr(vals);
        }
        env = cdr(env);
    }

    fprintf(stderr, "unbound variable: %s\n", var->str_val);
    exit(1);
}

void add_procedure(char* name, Object *proc(Object *args)) {
    define_variable(new_symbol(name),
                    new_primitive(proc),
                    global_env);
}

void init() {
    empty_list = new_object(TYPE_EMPTYLIST);

    true_obj = new_object(TYPE_BOOL);
    true_obj->bool_val = true;
    false_obj = new_object(TYPE_BOOL);
    false_obj->bool_val = false;

    global_env = extend_environment(empty_list, empty_list, empty_list);

    symbols_head  = empty_list;
    quote_symbol  = new_symbol("quote");
    define_symbol = new_symbol("define");
    set_symbol    = new_symbol("set!");
    ok_symbol     = new_symbol("ok");
    if_symbol     = new_symbol("if");
    lambda_symbol = new_symbol("lambda");
    cond_symbol   = new_symbol("cond");
    else_symbol   = new_symbol("else");
    apply_symbol  = new_symbol("apply");

    add_procedure("+",        _proc_add);
    add_procedure("-",        _proc_sub);
    add_procedure("*",        _proc_mul);
    add_procedure("/",        _proc_div);
    add_procedure("=",        _proc_equals);
    add_procedure("not",      _proc_not);

    add_procedure("null?",    _proc_is_null);
    add_procedure("eq?",      _proc_is_eq);
    add_procedure("number?",  _proc_is_number);
    add_procedure("string?",  _proc_is_string);
    add_procedure("symbol?",  _proc_is_symbol);
    add_procedure("pair?",    _proc_is_pair);
    
    add_procedure("list",     _proc_list);
    add_procedure("car",      _proc_car);
    add_procedure("cdr",      _proc_cdr);
    add_procedure("cons",     _proc_cons);
    add_procedure("set-car!", _proc_set_car);
    add_procedure("set-cdr!", _proc_set_cdr);
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
    char buf[BUF_MAX];
    int value;
    int sign = 1;
    int c = getc(ls->stream);

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

        case '0'...'9':
            ungetc(c, ls->stream);
            value = read_int(ls->stream);
            ls->token.kind = TK_INT;
            ls->token.int_val = sign * value;
            break;

        case '\"':
            c = getc(ls->stream);
            int len = 0;
            while (c != EOF && c != '\"') {
                if (len == BUF_MAX - 1) {
                    fprintf(stderr, "exceeded max buffer length\n");
                    exit(1);
                }

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
        case '+': case '-': case '*': case '/': case '=':
        case 'A'...'Z':
        case 'a'...'z': {
            // parse as number if digits follow minus sign
            if (c == '-' && isdigit(peek(ls->stream))) {
                sign = -1;
                value = read_int(ls->stream);
                ls->token.kind = TK_INT;
                ls->token.int_val = sign * value;
                break;
            }

            // otherwise, parse as symbol
            int len = 0;
            while (isalnum(c) || valid_chars[c]) {
                if (len == BUF_MAX - 1) {
                    fprintf(stderr, "exceeded max buffer length\n");
                    exit(1);
                }

                buf[len++] = c;
                c = getc(ls->stream);
            }
            ungetc(c, ls->stream);
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

/* Eval/Apply */

Object* list_of_values(Object* exps, Object* env) {
    if (exps == empty_list) return empty_list;

    return cons(eval(car(exps), env),
                list_of_values(cdr(exps), env));
}

Object* eval(Object* exp, Object* env) {
    switch (type(exp)) {
        
        // self-evaluating
        case TYPE_BOOL:
        case TYPE_INT:
        case TYPE_STRING:
            return exp;

        case TYPE_SYMBOL:
            return lookup_variable(exp, env);

        case TYPE_PAIR: {
            Object* tag = car(exp);

            if (tag == quote_symbol)
                return cadr(exp);

            if (tag == define_symbol) {
                define_variable(cadr(exp), eval(caddr(exp), env), env);
                return ok_symbol;
            }

            if (tag == set_symbol) {
                set_variable_value(cadr(exp), eval(caddr(exp), env), env);
                return ok_symbol;
            }

            if (tag == if_symbol) {
                if (eval(cadr(exp), env) != false_obj) {
                    return eval(caddr(exp), env);
                } else {
                    if (cdddr(exp) == empty_list)
                        return false_obj;

                    Object* alt = cadddr(exp);
                    return eval(alt, env);
                }
            }

            if (tag == cond_symbol) {
                Object* clauses = cdr(exp);
                while (car(clauses) != empty_list) {
                    if (caar(clauses) == else_symbol ||
                        eval(caar(clauses), env) == true_obj)
                        return eval(cadar(clauses), env);
                    clauses = cdr(clauses);
                }
                return NULL;
            }

            if (tag == lambda_symbol) {
                Object* params = cadr(exp);
                Object* body = cddr(exp);
                return new_procedure(params, body, env);
            }

            if (tag == apply_symbol) {
                Object* proc = eval(cadr(exp), env);
                Object* args = list_of_values(eval(caddr(exp), env), env);
                return apply(proc, args);
            }

            // procedure application
            Object* proc = eval(car(exp), env);
            Object* args = list_of_values(cdr(exp), env);
            return apply(proc, args);
        }
        default:
            fprintf(stderr, "unexpected type: [%s]\n", type_names[type(exp)]);
            exit(1);
    }
}

Object* apply(Object* proc, Object* args) {
    if (type(proc) == TYPE_PRIMITIVE)
        return proc->func(args);
    
    assert(type(proc) == TYPE_PROCEDURE, "expected compound procedure");
    Object* new_env = extend_environment(proc->params, args, proc->env);
    Object* body = proc->body;
    Object* result;
    while (body != empty_list) {
        result = eval(car(body), new_env);
        body = cdr(body);
    }
    return result;
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
        case TYPE_PROCEDURE:
        case TYPE_PRIMITIVE: printf("#<procedure>"); break;
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
            fprintf(stderr, "unexpected type: [%s]\n", type_names[type(obj)]);
            exit(1);
    }
}

/* Main */

void eval_all(LexState* ls) {
    next_token(ls);
    while (ls->token.kind != TK_EOF) {
        Object* exp = parse_exp(ls);
        Object* result = eval(exp, global_env);
        if (result) {
            print_object(result);
            printf("\n");
        }
    }
}

void skip_repl_space(FILE* stream) {
    int c = getc(stream);
    while (c == ' ') {
        c = getc(stream);
    }
    ungetc(c, stream);
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
                if (len == BUF_MAX - 1) {
                    fprintf(stderr, "exceeded max buffer length\n");
                    exit(1);
                }

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
