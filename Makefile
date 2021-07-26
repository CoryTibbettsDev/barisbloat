PROGRAMNAME := barisbloat
VERSION ?= $(shell git tag || printf "0.1")
LONGVERSION ?= $(shell git describe --tags || printf "0.1")

TARGETOS := $(shell uname -s)

RM ?= rm -f
CP ?= cp -f

# User compiled programs go in /usr/local/bin link below has good explination
# https://unix.stackexchange.com/questions/8656/usr-bin-vs-usr-local-bin-on-linux
PREFIX ?= /usr/local
DESTDIR ?= $(PREFIX)/bin

CFLAGS += -std=c99 -Wall
CPPFLAGS += -DPROGRAMNAME="\"$(PROGRAMNAME)\"" -DVERSION="\"$(VERSION)\""
# Extra flags for debugging
CFDEBUG = -g -pedantic -Wno-deprecated-declarations -Wunused-parameter -Wlong-long \
		  -Wsign-conversion -Wconversion -Wimplicit-function-declaration
# Included libraries
LDFLAGS += -lc -lxcb -lxcb-util -lxcb-randr# -lxcb-xinerama
# If Linux we add bsd c standard library
ifeq ($(TARGETOS),Linux)
	LDFLAGS += -lbsd
endif

EXEC := $(PROGRAMNAME)
SRC := main.c
OBJ := $(SRC:.c=.o)

.PHONY: all clean install uninstall

all: ${EXEC}

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

${EXEC}: ${OBJ}
	$(CC) ${LDFLAGS} -o $@ $<

test: CFLAGS += $(CFDEBUG)
test: all

config:
	$(CP) config.def.h config.h

install: all
	mkdir -p ${DESTDIR}
	cp -f ${EXEC} ${DESTDIR}
	chmod 755 ${DESTDIR}/${EXEC}

uninstall:
	${RM} ${DESTDIR}/${EXEC}

clean:
	${RM} ${OBJ} ${EXEC}
