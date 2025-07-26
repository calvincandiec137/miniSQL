CC = gcc
CFLAGS = -Iinclude -g -Wall
VPATH = src/btree:src/pager:tests
BUILD_DIR = build
BIN_DIR = bin
TARGET = $(BIN_DIR)/test_btree

SRCS = btree.c pager.c test_btree.c
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all test clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

test: all
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)