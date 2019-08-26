#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Object */

typedef enum ObjectType {
    TYPE_INT
} ObjectType;

typedef struct Object {
    ObjectType type;
    union {
        int int_val;
    };
} Object;

Object* new_object(ObjectType type) {
    Object* object = malloc(sizeof(Object));
    object->type = type;
    return object;
}

/* Reading */

void skip_whitespace(FILE* stream) {
    char c = getc(stream);
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
    return value;
}

Object* read(FILE* stream) {
    skip_whitespace(stream);
    char c = getc(stream);

    if (c == '-') {
        int value = read_int(stream);
        Object* object = new_object(TYPE_INT);
        object->int_val = -value;
        return object;
    } else if (isdigit(c)) {
        ungetc(c, stream);
        int value = read_int(stream);
        Object* object = new_object(TYPE_INT);
        object->int_val = value;
        return object;
    } else {
        fprintf(stderr, "unexpected character: %c\n", c);
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

int main(void) {
    printf("Welcome to Bootstrap Scheme v0.1\n");

    while (true) {
        printf("> ");
        print_object(read(stdin));
        printf("\n");
    }

    return 0;
}
