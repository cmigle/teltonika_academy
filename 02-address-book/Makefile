CPPFLAGS:= 
CFLAGS := -Wall -Wextra
LDFLAGS := 

TARGET := addressbook
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

debug: CFLAGS += -g
debug: all

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean debug
