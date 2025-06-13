CC = gcc
CFLAGS = -Iinclude -ggdb

LIB_SRC = $(wildcard src/lib/*.c)
LIB_OBJ = $(patsubst src/lib/%.c, build/lib/%.o, $(LIB_SRC))

.PHONY: all clean

all: bin/server bin/client bin/test

build/lib/%.o: src/lib/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

bin/server: $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ src/server.c -o $@

bin/client: $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ src/client.c -o $@

bin/test: $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ test/ts_queue.c -o $@

clean:
	rm -r bin/* build/*
