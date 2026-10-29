CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pedantic -O2 -Iinclude -Ilib
LDFLAGS = -lsqlite3 -lws2_32

SRC = $(wildcard src/*.c) $(wildcard lib/*.c)
TARGET = server.exe

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o $(TARGET) ../tests/*.exe

test-toolchain:
	$(CC) $(CFLAGS) ../tests/test_toolchain.c -o ../tests/test_toolchain.exe $(LDFLAGS)
	../tests/test_toolchain.exe

.PHONY: all clean test-toolchain
