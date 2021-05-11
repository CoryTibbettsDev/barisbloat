# barisbloat Makefile
VERSION = 0.1
PROGRAMNAME=barisbloat

# User compiled programs go in /usr/local/bin link below has good explination
# https://unix.stackexchange.com/questions/8656/usr-bin-vs-usr-local-bin-on-linux
DESTDIR = /usr/local/bin

CFLAGS = -std=c99 -Wall -pedantic -Wno-deprecated-declarations -Os ${CPPFLAGS}
CPPFLAGS = -DPROGRAMNAME="\"$(PROGRAMNAME)\"" -DVERSION="\"$(VERSION)\""
# Extra flags for debugging
CFDEBUG = -g -Wunused-parameter -Wlong-long -Wsign-conversion \
		   -Wconversion -Wimplicit-function-declaration
# Included libraries
LDFLAGS = -lc -lxcb -lxcb-randr# -lxcb-xinerama
# If Linux we add bsd c standard library
TARGETOS = $(shell uname -s)
ifeq ($(TARGETOS),Linux)
	LDFLAGS += -lbsd
endif

EXEC = ${PROGRAMNAME}
SRC = main.c
OBJ = ${SRC:.c=.o}

all: ${EXEC}

.c.o:
	${CC} -c ${CFLAGS} $<

${EXEC}: ${OBJ}
	$(CC) -o $@ $< ${LDFLAGS}

config:
	cp config.def.h config.h

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
