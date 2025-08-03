
# B-Tree Implementation Documentation

## Overview

This is a disk-based B-Tree implementation in C that provides key-value storage with variable-length values. The implementation supports insertions, searches, and sequential traversal with automatic node splitting when capacity is exceeded.

## Architecture

The B-Tree consists of two main components:

-   **Pager**: Manages page allocation and memory mapping
-   **B-Tree**: Implements the tree structure with internal and leaf nodes

### Key Features

-   Variable-length values in leaf nodes
-   Automatic node splitting on overflow
-   Sequential traversal via leaf node chaining
-   Duplicate key rejection
-   Page-based storage (4KB pages)

## Constants and Configuration

```c
#define PAGE_SIZE 4096
#define LEAF_NODE_MAX_CELLS 13          // Conservative estimate for variable-length cells
#define INTERNAL_NODE_MAX_CELLS         // Calculated based on fixed cell size
#define LEAF_NODE_LEFT_SPLIT_COUNT      // Half of max cells
#define LEAF_NODE_RIGHT_SPLIT_COUNT     // Remaining half

```

## Data Structures

### BTree

```c
struct BTree {
    Pager* pager;           // Page manager
    page_num_t root_page_num; // Root page number (always 0)
};

```

### BTreeCursor

```c
struct BTreeCursor {
    BTree* btree;           // Reference to tree
    page_num_t page_num;    // Current page
    uint32_t cell_num;      // Current cell position
    bool end_of_table;      // End marker for traversal
};

```

## Node Layout

### Common Header (For all nodes) 
|Offset|Size|Field|
|--|--|--|
| 0 |1  |Node Type  |
|1|1|Is root flag|
|2 |4 |Parent page number|

### Leaf Node Layout
|Offset|Size|Field|
|--|--|--|
| 7 |2  |Number of cells  |
|9|4|Next leaf page number|
|13+ |Variable |Cells (key + value_size + value)|

### Internal Node Layout
|Offset|Size|Field|
|--|--|--|
| 7 |2  |Number of keys  |
|9|4|Right child page number|
|13+ |8 x n |Cells (child_page + key) |





## Function Reference

### [Core Operations](#core-operations-1)

-   [`btree_open()`](#btree_open) - Initialize B-Tree
-   [`btree_close()`](#btree_close) - Cleanup B-Tree
-   [`btree_insert()`](#btree_insert) - Insert key-value pair
-   [`btree_find()`](#btree_find) - Search for key

### [Cursor Operations](#cursor-operations-1)

-   [`btree_start()`](#btree_start) - Get cursor to first record
-   [`btree_cursor_advance()`](#btree_cursor_advance) - Move to next record
-   [`btree_cursor_get_value()`](#btree_cursor_get_value) - Retrieve current value

### [Node Management](#node-management-1)

-   [`initialize_leaf_node()`](#initialize_leaf_node) - Setup new leaf node
-   [`initialize_internal_node()`](#initialize_internal_node) - Setup new internal node
-   [`leaf_node_split_and_insert()`](#leaf_node_split_and_insert) - Split full leaf
-   [`internal_node_split_and_insert()`](#internal_node_split_and_insert) - Split full internal node
-   [`create_new_root()`](#create_new_root) - Create new root after split

### [Search Functions](#search-functions-1)

-   [`leaf_node_find()`](#leaf_node_find) - Binary search in leaf
-   [`internal_node_find()`](#internal_node_find) - Navigate internal node
-   [`internal_node_find_child()`](#internal_node_find_child) - Find child index

### [Utility Functions](#utility-functions-1)

-   [`get_node_max_key()`](#get_node_max_key) - Get largest key in node
-   [`serialize_leaf_value()`](#serialize_leaf_value) - Pack key-value data
-   [`update_internal_node_key()`](#update_internal_node_key) - Update parent key
-   [`get_leaf_cell_size()`](#get_leaf_cell_size) - Calculate cell size

### [Node Accessors](#node-accessors-1)

-   Node type and metadata accessors
-   Leaf node cell accessors
-   Internal node key/child accessors

----------

## Core Operations

### `btree_open()`

```c
BTree* btree_open(Pager* pager);

```

Initializes a new B-Tree instance using the provided pager. If the database is empty, creates an initial root leaf node.

**Parameters:**

-   `pager`: Initialized pager instance

**Returns:** New BTree instance or NULL on failure

----------

### `btree_close()`

```c
void btree_close(BTree* btree);

```

Releases B-Tree resources. Note: Does not close the pager.

**Parameters:**

-   `btree`: B-Tree instance to close

----------

### `btree_insert()`

```c
int btree_insert(BTree* btree, uint32_t key, void* value, uint32_t value_size);

```

Inserts a key-value pair into the tree. Automatically handles node splits when necessary.

**Parameters:**

-   `btree`: Target B-Tree
-   `key`: 32-bit key (must be unique)
-   `value`: Value data
-   `value_size`: Size of value in bytes

**Returns:**

-   `0`: Success
-   `-1`: Duplicate key error

**Behavior:**

1.  Finds insertion point using `btree_find()`
2.  Checks for duplicate keys
3.  Calls `leaf_node_insert()` which may trigger splits
4.  Updates parent nodes if splits occur

----------

### `btree_find()`

```c
BTreeCursor* btree_find(BTree* btree, uint32_t key);

```

Searches for a key and returns a cursor positioned at the key or insertion point.

**Parameters:**

-   `btree`: Target B-Tree
-   `key`: Key to search for

**Returns:** Cursor positioned at key or insertion point

**Algorithm:**

1.  Start at root node
2.  If internal node, use `internal_node_find()` to navigate
3.  If leaf node, use `leaf_node_find()` for binary search
4.  Return cursor at exact match or insertion position

----------

## Cursor Operations

### `btree_start()`

```c
BTreeCursor* btree_start(BTree* btree);

```

Creates a cursor positioned at the first (smallest) key in the tree.

**Algorithm:**

1.  Start at root
2.  Follow leftmost child pointers through internal nodes
3.  Position at first cell of leftmost leaf

----------

### `btree_cursor_advance()`

```c
void btree_cursor_advance(BTreeCursor* cursor);

```

Moves cursor to the next record in sorted order.

**Behavior:**

1.  Increment cell number
2.  If at end of leaf, follow `next_leaf` pointer
3.  Reset cell number to 0 in new leaf
4.  Set `end_of_table` if no more leaves

----------

### `btree_cursor_get_value()`

```c
void btree_cursor_get_value(BTreeCursor* cursor, void* value_buffer, 
                           uint32_t buffer_size, uint32_t* value_size);

```

Retrieves the value at the cursor's current position.

**Parameters:**

-   `cursor`: Positioned cursor
-   `value_buffer`: Destination buffer
-   `buffer_size`: Size of destination buffer
-   `value_size`: [OUT] Actual value size

**Safety:** Only copies data if buffer is large enough, but always reports actual size.

----------

## Node Management

### `initialize_leaf_node()`

```c
void initialize_leaf_node(void* node);

```

Sets up a new leaf node with default values:

-   Node type: `NODE_LEAF`
-   Is root: `false`
-   Cell count: `0`
-   Next leaf: `0` (no sibling)

----------

### `initialize_internal_node()`

```c
void initialize_internal_node(void* node);

```

Sets up a new internal node:

-   Node type: `NODE_INTERNAL`
-   Is root: `false`
-   Key count: `0`

----------

### `leaf_node_split_and_insert()`

```c
void leaf_node_split_and_insert(BTreeCursor* cursor, uint32_t key, 
                                void* value, uint32_t value_size);

```

Handles leaf node overflow by splitting into two nodes.

**Algorithm:**

1.  Create temporary arrays for all data (existing + new)
2.  Insert new key-value at correct position
3.  Split at `LEAF_NODE_LEFT_SPLIT_COUNT`
4.  Distribute cells between old and new nodes
5.  Update leaf chain pointers
6.  Handle parent insertion (may create new root)

**Key Insight:** Maintains sorted order and proper leaf chaining for sequential traversal.

----------

### `internal_node_split_and_insert()`

```c
void internal_node_split_and_insert(BTree* btree, page_num_t parent_page_num, 
                                   page_num_t child_page_num);

```

Splits an internal node when it exceeds capacity.

**Algorithm:**

1.  Create temporary arrays for keys and children
2.  Insert new child at correct position to maintain order
3.  Split at middle point
4.  Promote middle key to parent
5.  Update all child parent pointers
6.  Handle root creation if necessary

**Critical:** Maintains B-Tree property that internal keys represent maximum keys in left subtrees.

----------

### `create_new_root()`

```c
page_num_t create_new_root(BTree* btree, page_num_t right_child_page_num);

```

Creates a new root node when the current root splits.

**Process:**

1.  Copy old root data to new page (becomes left child)
2.  Initialize old root as internal node
3.  Set up single key separating two children
4.  Update parent pointers

----------

## Search Functions

### `leaf_node_find()`

```c
BTreeCursor* leaf_node_find(BTree* btree, page_num_t page_num, uint32_t key);

```

Performs binary search within a leaf node.

**Returns:** Cursor at exact match or insertion position

----------

### `internal_node_find()`

```c
BTreeCursor* internal_node_find(BTree* btree, page_num_t page_num, uint32_t key);

```

Navigates through internal node to find appropriate child.

**Algorithm:**

1.  Use `internal_node_find_child()` to find child index
2.  Recursively search child node
3.  Return result from leaf level

----------

### `internal_node_find_child()`

```c
uint32_t internal_node_find_child(void* node, uint32_t key);

```

Binary search to find which child should contain the key.

**Logic:** Keys in internal nodes represent the maximum key in the left child, so we find the first key ≥ target.

----------

## Utility Functions

### `get_node_max_key()`

```c
uint32_t get_node_max_key(void* node);

```

Returns the largest key in a node:

-   **Leaf nodes:** Key of last cell
-   **Internal nodes:** Last key in keys array

----------

### `serialize_leaf_value()`

```c
void serialize_leaf_value(void* destination, uint32_t key, 
                         void* value, uint32_t value_size);

```

Packs key-value data into leaf cell format:

```
[4 bytes: key][4 bytes: value_size][value_size bytes: value]

```

----------

### `update_internal_node_key()`

```c
void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key);

```

Updates a key in an internal node (used after child splits change maximum keys).

----------

### `get_leaf_cell_size()`

```c
uint32_t get_leaf_cell_size(void* node, uint32_t cell_num);

```

Calculates the total size of a leaf cell including variable-length value.

**Formula:** `sizeof(key) + sizeof(value_size) + actual_value_size`

----------

## Node Accessors

### Node Metadata

-   `get_node_type()` / `set_node_type()` - Node type (leaf/internal)
-   `is_node_root()` / `set_node_root()` - Root flag
-   `node_parent()` - Parent page pointer

### Leaf Node Accessors

-   `leaf_node_num_cells()` - Cell count pointer
-   `leaf_node_next_leaf()` - Next leaf pointer
-   `leaf_node_cell()` - Cell pointer by index
-   `leaf_node_key()` - Key pointer in cell
-   `leaf_node_value_size()` - Value size pointer
-   `leaf_node_value()` - Value data pointer

### Internal Node Accessors

-   `internal_node_num_keys()` - Key count pointer
-   `internal_node_right_child()` - Rightmost child pointer
-   `internal_node_cell()` - Cell pointer by index
-   `internal_node_child()` - Child page by index
-   `internal_node_key()` - Key by index

----------

## Testing

The test suite (`test_btree.c`) provides comprehensive validation:

### Test Cases

1.  **Basic Operations** - Simple insert/retrieve
2.  **Sequential Insertion** - Ordered keys (worst case for splits)
3.  **Random Insertion** - Shuffled keys
4.  **Duplicate Handling** - Key uniqueness enforcement
5.  **Stress Testing** - 100+ insertions with validation

### Key Features

-   **Tree Visualization** - `print_tree_structure()` shows node hierarchy
-   **Structure Validation** - `validate_tree_structure()` checks B-Tree properties
-   **Sequential Verification** - Ensures cursor traversal returns sorted data
-   **Split Testing** - Forces multiple node splits and validates integrity

### Usage

```bash
make test    # Builds and runs all tests

```

The test output includes tree structure dumps after major operations, making it easy to visualize the B-Tree's evolution during insertions and splits.

----------

## Implementation Notes

### Memory Management

-   Pages are allocated through the pager interface
-   Variable-length values require careful size calculations
-   Node splits use temporary arrays to avoid corruption

### Concurrency

-   This implementation is single-threaded
-   No locking or atomic operations

### Error Handling

-   Duplicate key insertion returns error code
-   Buffer overflow protection in cursor operations
-   Bounds checking in node accessors

### Performance Characteristics

-   **Search:** O(log n) where n is number of keys
-   **Insert:** O(log n) average, may require multiple splits
-   **Sequential Scan:** O(n) via leaf chain traversal
-   **Space:** Variable depending on value sizes
