TARGET := liblinklist.so
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

CFLAGS := -Wall -Wextra -fPIC

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -shared -o $@ $^ 

debug: CFLAGS += -g
debug: all

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean debug