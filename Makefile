TARGET_EXEC ?= edgedetect

CC = clang

BUILD_DIR ?= ./build
SRC_DIRS ?= ./src

SRCS := $(shell find $(SRC_DIRS) -name *.c)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)

INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

CFLAGS = -std=gnu17 -Wall -pg -Wextra -pedantic -O3 
LDFLAGS = -lm

ifndef asan
	ASAN = 
else 
	ASAN = -fsanitize=address
endif

$(TARGET_EXEC): $(OBJS)
	$(CC) $(OBJS) -g -o $@ $(LDFLAGS) $(ASAN) 

$(BUILD_DIR)/%.c.o: %.c
	$(shell mkdir -p $(dir $@))
	$(CC) $(CFLAGS) -c $< -o $@ $(ASAN)

.PHONY: clean
clean:
	$(RM) -r $(BUILD_DIR)


