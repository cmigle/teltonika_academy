CFLAGS := -Wall -Wextra -g -I../lib
LDFLAGS := -L../lib -Wl,-rpath,'$$ORIGIN/../lib'
LDLIBS := -l linklist

TARGET := addressbook
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean