# bloatbar Makefile
VERSION = 0.1

CC ?= gcc
CFLAGS += -Wall -std=c99 -DVERSION="\"$(VERSION)\""
# Include xcb libraries
LDFLAGS += -lxcb -lxcb-randr # -lxcb-xinerama
CFDEBUG = -g3 -pedantic -Wall -Wunused-parameter -Wlong-long \
          -Wsign-conversion -Wconversion -Wimplicit-function-declaration

EXEC = bloatbar
SRC = main.c util.c components.c
OBJ = ${SRC:.c=.o}

.c.o:
	${CC} -c ${CFLAGS} $<

${EXEC}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

all: ${EXEC}

clean:
	rm -f ${OBJ}
	rm -f ${EXEC}

.PHONY: all clean install uninstall
