CC      = gcc
CFLAGS  = -Wall -Wextra -O2

SOURCES = $(wildcard *.c)
EXECUTABLES = $(SOURCES:.c=)

all: $(EXECUTABLES)

%: %.c
	$(CC) $(CFLAGS) -o ./bin/$@ $^

clean:
	rm -f $(EXECUTABLES) *.o
