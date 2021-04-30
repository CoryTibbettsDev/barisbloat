# barisbloat Makefile
VERSION = 0.1
PROGRAMNAME="barisbloat"

CFLAGS = -std=c99 -Wall -pedantic -Wno-deprecated-declarations -Os ${CPPFLAGS} ${CFDEBUG}
CPPFLAGS = -DPROGRAMNAME="\"$(PROGRAMNAME)\"" -DVERSION="\"$(VERSION)\"" \
		   -D_GNU_SOURCE -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L
CFDEBUG = -Wunused-parameter -Wlong-long -Wsign-conversion \
		   -Wconversion -Wimplicit-function-declaration
# Included libraries
LDFLAGS = -lxcb -lxcb-randr # -lxcb-xinerama
# If Linux we add bsd library for BSD's good functions like strlcpy
TARGETOS = $(shell uname -s)
ifeq ($(TARGETOS),Linux)
	LDFLAGS += -lbsd
endif

# User compiled programs go in /usr/local/bin link below has good explination
# https://unix.stackexchange.com/questions/8656/usr-bin-vs-usr-local-bin-on-linux
DESTDIR = /usr/local/bin

EXEC = barisbloat
SRC = main.c
OBJ = ${SRC:.c=.o}

all: ${EXEC}

.c.o:
	${CC} -c ${CFLAGS} $<

${EXEC}: ${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

config.h:
	cp config.def.h $@

install: all
	mkdir -p ${DESTDIR}
	cp -f ${EXEC} ${DESTDIR}
	chmod 755 ${DESTDIR}/${EXEC}

uninstall:
	${RM} ${DESTDIR}/${EXEC}

clean:
	${RM} ${OBJ}
	${RM} ${EXEC}

.PHONY: all clean options install uinstall
