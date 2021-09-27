# GNU Makefile

# This makefile will likely not build when using a non GNU Make

PROG := barisbloat
VERSION ?= 0.1

RM ?= rm -vf
CP ?= cp -vf

PREFIX ?= /usr/local
DESTDIR ?=

CFLAGS += -Wall
CPPFLAGS += -DPROG="\"$(PROG)\"" -DVERSION="\"$(VERSION)\""
# Extra flags for debugging
CFDEBUG = -g -pedantic -Wno-deprecated-declarations -Wunused-parameter -Wlong-long \
		  -Wsign-conversion -Wconversion -Wimplicit-function-declaration
# Included libraries
LDFLAGS += -lc -lxcb -lxcb-util -lxcb-randr# -lxcb-xinerama

SRC := main.c
OBJ := $(SRC:.c=.o)

.PHONY: all clean install uninstall

all: $(PROG)

$(PROG): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $(PROG)

$(OBJ): %.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $<

test: CFLAGS += $(CFDEBUG)
test: all

config:
	$(CP) config.def.h config.h

install: all
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(PROG) $(DESTDIR)$(PREFIX)/bin/$(PROG)

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(PROG)

clean:
	$(RM) $(PROG) *.o
