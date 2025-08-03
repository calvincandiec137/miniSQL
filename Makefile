CC = gcc
CFLAGS = -Iinclude -g -Wall -Wextra -std=c99
BUILD_DIR = build
BIN_DIR = bin

# Source files
BTREE_SRC = src/btree/btree.c
PAGER_SRC = src/pager/pager.c
TEST_SRC = tests/test_btree.c

# Object files
BTREE_OBJ = $(BUILD_DIR)/btree.o
PAGER_OBJ = $(BUILD_DIR)/pager.o
TEST_OBJ = $(BUILD_DIR)/test_btree.o

# Targets
TEST_BIN = $(BIN_DIR)/test_btree

.PHONY: all clean test

all: $(TEST_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BTREE_OBJ): $(BTREE_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(PAGER_OBJ): $(PAGER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_OBJ): $(TEST_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(BTREE_OBJ) $(PAGER_OBJ) $(TEST_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_BIN)
	@echo "Running comprehensive B-Tree tests..."
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f *.db