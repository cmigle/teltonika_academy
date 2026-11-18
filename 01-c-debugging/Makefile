CFLAGS ?= -Wall -Wextra

SRC := $(wildcard *.c)
BINS := $(SRC:.c=)

.PHONY: all clean

all: $(BINS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(BINS) copied_file.txt
