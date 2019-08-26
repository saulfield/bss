SRC_FILES = bss.c
CC_FLAGS = -Wall -Wextra -g -std=c99
CC = gcc

.PHONY: clean

all:
	${CC} ${SRC_FILES} ${CC_FLAGS} -o bss

clean:
	rm bss