CC = gcc
CFLAGS = -pedantic -Wall -std=c99 -O2 -D_GNU_SOURCE

PROG = prcat
OBJECTS = readfile.o parser.o setup.o connect.o tunnel.o proxy.o base64.o \
	xgetpass.o askpass.o buffer.o

VERSION = version.h
MKVERSION = ../tools/mkversion.sh

SOURCES = $(OBJECTS:%.o=%.c) $(PROG:%=%.c)
HEADERS = $(OBJECTS:%.o=%.h)

all: $(PROG)

# potentially updates the version file if any dependency was modified
$(VERSION): $(SOURCES) $(HEADERS)
	$(MKVERSION) $@ $(BUILD_VERSION)

# default dependencies and compile for objects
$(OBJECTS): %.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

# default dependencies and compile for prog
$(PROG:%=%.o): %.o: %.c
	 $(CC) $(CFLAGS) -c $< -o $@

# default dependencies and link method for prog
$(PROG): %: %.o $(OBJECTS)
	 $(CC) $(CFLAGS) $< $(OBJECTS) -o $@

# additional version dependencies for objects
setup.o: $(VERSION)

# additional header dependencies for objects
askpass.o: xgetpass.h
parser.o: readfile.h porting.h
proxy.o: base64.h porting.h
readfile.o: porting.h
setup.o: parser.h

# additional header dependencies for prog
prcat.o: askpass.h connect.h proxy.h setup.h tunnel.h

.PHONY: clean
clean:
	rm -f $(VERSION) $(OBJECTS) $(PROG:%=%.o) $(PROG)

