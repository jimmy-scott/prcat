CC = gcc
CFLAGS = -pedantic -Wall -std=c99 -O2

PROGS = prcat
OBJECTS = readfile.o parser.o setup.o connect.o tunnel.o proxy.o base64.o \
	xgetpass.o askpass.o

all: $(PROGS)

$(OBJECTS): %.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

$(PROGS): %: %.c $(OBJECTS)
	 $(CC) $(CFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f $(OBJECTS) $(PROGS)

