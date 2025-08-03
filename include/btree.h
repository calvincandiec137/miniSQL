#ifndef BTREE_H
#define BTREE_H

#include "pager.h"
#include <stdint.h>
#include <stdbool.h>

// Forward declarations
typedef struct BTree BTree;
typedef struct BTreeCursor BTreeCursor;

// Node types
typedef enum { NODE_INTERNAL, NODE_LEAF } NodeType;

// Main B-tree operations
BTree* btree_open(Pager* pager);
void btree_close(BTree* btree);
int btree_insert(BTree* btree, uint32_t key, void* value, uint32_t value_size);

// Cursor operations
BTreeCursor* btree_find(BTree* btree, uint32_t key);
BTreeCursor* btree_start(BTree* btree);
void btree_cursor_advance(BTreeCursor* cursor);
void btree_cursor_get_value(BTreeCursor* cursor, void* value_buffer, uint32_t buffer_size, uint32_t* value_size);

// Internal helper functions (exposed for testing)
BTreeCursor* leaf_node_find(BTree* btree, page_num_t page_num, uint32_t key);
BTreeCursor* internal_node_find(BTree* btree, page_num_t page_num, uint32_t key);
uint32_t get_node_max_key(void* node);
void leaf_node_split_and_insert(BTreeCursor* cursor, uint32_t key, void* value, uint32_t value_size);
void internal_node_insert(BTree* btree, page_num_t parent_page_num, page_num_t child_page_num);
uint32_t internal_node_find_child(void* node, uint32_t key);
void update_internal_node_key(void* node, uint32_t old_key, uint32_t new_key);
page_num_t create_new_root(BTree* btree, page_num_t right_child_page_num);

// Utility functions
uint32_t get_leaf_cell_size(void* node, uint32_t cell_num);
void serialize_leaf_value(void* destination, uint32_t key, void* value, uint32_t value_size);

// Node accessor functions (needed for testing)
NodeType get_node_type(void* node);
void set_node_type(void* node, NodeType type);
bool is_node_root(void* node);
void set_node_root(void* node, bool is_root);
uint32_t* node_parent(void* node);

// Leaf node accessors
uint16_t* leaf_node_num_cells(void* node);
uint32_t* leaf_node_next_leaf(void* node);
void* leaf_node_cell(void* node, uint32_t cell_num);
uint32_t* leaf_node_key(void* node, uint32_t cell_num);
uint32_t* leaf_node_value_size(void* node, uint32_t cell_num);
void* leaf_node_value(void* node, uint32_t cell_num);

// Internal node accessors
uint16_t* internal_node_num_keys(void* node);
uint32_t* internal_node_right_child(void* node);
uint32_t* internal_node_cell(void* node, uint32_t cell_num);
uint32_t* internal_node_child(void* node, uint32_t child_num);
uint32_t* internal_node_key(void* node, uint32_t key_num);

// Node initialization
void initialize_leaf_node(void* node);
void initialize_internal_node(void* node);

// Struct definitions (for testing access)
struct BTree {
    Pager* pager;
    page_num_t root_page_num;
};

struct BTreeCursor {
    BTree* btree;
    page_num_t page_num;
    uint32_t cell_num;
    bool end_of_table;
};

#endif