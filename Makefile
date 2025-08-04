CC = gcc
CFLAGS = -Iinclude -g -Wall -Wextra -std=c99
BUILD_DIR = build
BIN_DIR = bin

# Source files
MAIN_SRC = src/main.c
BTREE_SRC = src/btree/btree.c
PAGER_SRC = src/pager/pager.c
VM_SRC = src/vm/vm.c
REPL_SRC = src/repl/repl.c
TEST_BTREE_SRC = tests/test_btree.c
TEST_VM_SRC = tests/test_vm.c

# Object files
MAIN_OBJ = $(BUILD_DIR)/main.o
BTREE_OBJ = $(BUILD_DIR)/btree.o
PAGER_OBJ = $(BUILD_DIR)/pager.o
VM_OBJ = $(BUILD_DIR)/vm.o
REPL_OBJ = $(BUILD_DIR)/repl.o
TEST_BTREE_OBJ = $(BUILD_DIR)/test_btree.o
TEST_VM_OBJ = $(BUILD_DIR)/test_vm.o

# Targets
MINISQL_BIN = $(BIN_DIR)/miniSQL
TEST_BTREE_BIN = $(BIN_DIR)/test_btree
TEST_VM_BIN = $(BIN_DIR)/test_vm

.PHONY: all clean test test-btree test-vm

all: $(MINISQL_BIN) $(TEST_BTREE_BIN) $(TEST_VM_BIN)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(MAIN_OBJ): $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BTREE_OBJ): $(BTREE_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(PAGER_OBJ): $(PAGER_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(VM_OBJ): $(VM_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(REPL_OBJ): $(REPL_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BTREE_OBJ): $(TEST_BTREE_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_VM_OBJ): $(TEST_VM_SRC) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(MINISQL_BIN): $(MAIN_OBJ) $(BTREE_OBJ) $(PAGER_OBJ) $(VM_OBJ) $(REPL_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_BTREE_BIN): $(BTREE_OBJ) $(PAGER_OBJ) $(TEST_BTREE_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

$(TEST_VM_BIN): $(VM_OBJ) $(BTREE_OBJ) $(PAGER_OBJ) $(TEST_VM_OBJ) | $(BIN_DIR)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TEST_BTREE_BIN) $(TEST_VM_BIN)
	@echo "Running all tests..."
	./$(TEST_BTREE_BIN)
	./$(TEST_VM_BIN)

test-btree: $(TEST_BTREE_BIN)
	@echo "Running comprehensive B-Tree tests..."
	./$(TEST_BTREE_BIN)

test-vm: $(TEST_VM_BIN)
	@echo "Running VM tests..."
	./$(TEST_VM_BIN)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	rm -f *.db