SDK_DIR ?= ../tuya-iot-core-sdk

CPPFLAGS := -I. -I$(SDK_DIR)/include -I$(SDK_DIR)/interface -I$(SDK_DIR)/utils
CFLAGS := -Wall -Wextra
LDFLAGS := -L$(SDK_DIR)/build/lib -Wl,-rpath,$(SDK_DIR)/build/lib
LDLIBS := -llink_core -lmiddleware_implementation -lplatform_port -lutils_modules -lubus -lubox -lblobmsg_json -lpthread

TARGET := tuya_daemon
SRCS := $(wildcard *.c)
OBJS := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
    $(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

debug: CFLAGS += -g
debug: all

clean:
    rm -f $(TARGET) $(OBJS)

.PHONY: all clean debug