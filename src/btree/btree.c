#include "btree.h"
#include "common.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Common Node Header Layout
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint32_t COMMON_NODE_HEADER_SIZE =
    NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

// Leaf Node Header Layout
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint16_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_NEXT_LEAF_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_NEXT_LEAF_OFFSET =
    LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                       LEAF_NODE_NUM_CELLS_SIZE +
                                       LEAF_NODE_NEXT_LEAF_SIZE;

// Internal Node Header Layout
const uint32_t INTERNAL_NODE_NUM_KEYS_SIZE = sizeof(uint16_t);
const uint32_t INTERNAL_NODE_NUM_KEYS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_RIGHT_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_RIGHT_CHILD_OFFSET =
    INTERNAL_NODE_NUM_KEYS_OFFSET + INTERNAL_NODE_NUM_KEYS_SIZE;
const uint32_t INTERNAL_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE +
                                           INTERNAL_NODE_NUM_KEYS_SIZE +
                                           INTERNAL_NODE_RIGHT_CHILD_SIZE;

// Cell sizes
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_VALUE_SIZE_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CHILD_SIZE = sizeof(uint32_t);
const uint32_t INTERNAL_NODE_CELL_SIZE =
    INTERNAL_NODE_CHILD_SIZE + INTERNAL_NODE_KEY_SIZE;

// Calculate max cells for internal nodes (since their cells are fixed size)
const uint32_t INTERNAL_NODE_SPACE_FOR_CELLS =
    PAGE_SIZE - INTERNAL_NODE_HEADER_SIZE;
const uint32_t INTERNAL_NODE_MAX_CELLS =
    INTERNAL_NODE_SPACE_FOR_CELLS / INTERNAL_NODE_CELL_SIZE;

// For leaf nodes, cells are variable size. A hardcoded, conservative estimate
// is the simplest approach. A more complex engine would check actual byte
// usage.
const uint32_t LEAF_NODE_MAX_CELLS = 13; // for test

// Split counts
const uint32_t LEAF_NODE_RIGHT_SPLIT_COUNT = (LEAF_NODE_MAX_CELLS + 1) / 2;
const uint32_t LEAF_NODE_LEFT_SPLIT_COUNT =
    (LEAF_NODE_MAX_CELLS + 1) - LEAF_NODE_RIGHT_SPLIT_COUNT;

// Invalid page number
const uint32_t INVALID_PAGE_NUM = UINT32_MAX;

// Forward declarations
void leaf_node_insert(BTreeCursor *cursor, uint32_t key, void *value,
                      uint32_t value_size);
void internal_node_split_and_insert(BTree *btree, page_num_t parent_page_num,
                                    page_num_t child_page_num);

// Node accessor functions
NodeType get_node_type(void *node) {
  uint8_t value = *((uint8_t *)((char *)node + NODE_TYPE_OFFSET));
  return (NodeType)value;
}

void set_node_type(void *node, NodeType type) {
  uint8_t value = type;
  *((uint8_t *)((char *)node + NODE_TYPE_OFFSET)) = value;
}

bool is_node_root(void *node) {
  uint8_t value = *((uint8_t *)((char *)node + IS_ROOT_OFFSET));
  return (bool)value;
}

void set_node_root(void *node, bool is_root) {
  uint8_t value = is_root;
  *((uint8_t *)((char *)node + IS_ROOT_OFFSET)) = value;
}

uint32_t *node_parent(void *node) {
  return (uint32_t *)((char *)node + PARENT_POINTER_OFFSET);
}

// Leaf node accessors
uint16_t *leaf_node_num_cells(void *node) {
  return (uint16_t *)((char *)node + LEAF_NODE_NUM_CELLS_OFFSET);
}

uint32_t *leaf_node_next_leaf(void *node) {
  return (uint32_t *)((char *)node + LEAF_NODE_NEXT_LEAF_OFFSET);
}

// Helper functions for leaf node operations - FIXED: Better bounds checking
uint32_t get_leaf_cell_size(void *node, uint32_t cell_num) {
  uint32_t num_cells = *leaf_node_num_cells(node);
  if (cell_num >= num_cells) {
    return LEAF_NODE_KEY_SIZE +
           LEAF_NODE_VALUE_SIZE_SIZE; // Minimum size for safety
  }
  uint32_t value_size = *leaf_node_value_size(node, cell_num);
  return LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE_SIZE + value_size;
}

void *leaf_node_cell(void *node, uint32_t cell_num) {
  // Simple linear layout for cells
  uint32_t offset = LEAF_NODE_HEADER_SIZE;
  uint32_t num_cells = *leaf_node_num_cells(node);

  // FIXED: Bounds checking to prevent buffer overflow
  if (cell_num > num_cells) {
    // Return position where new cell would go
    cell_num = num_cells;
  }

  for (uint32_t i = 0; i < cell_num; i++) {
    offset += get_leaf_cell_size(node, i);
  }
  return (char *)node + offset;
}

uint32_t *leaf_node_key(void *node, uint32_t cell_num) {
  void *cell = leaf_node_cell(node, cell_num);
  return (uint32_t *)cell;
}

uint32_t *leaf_node_value_size(void *node, uint32_t cell_num) {
  void *cell = leaf_node_cell(node, cell_num);
  return (uint32_t *)((char *)cell + LEAF_NODE_KEY_SIZE);
}

void *leaf_node_value(void *node, uint32_t cell_num) {
  void *cell = leaf_node_cell(node, cell_num);
  return (char *)cell + LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE_SIZE;
}

// Internal node accessors
uint16_t *internal_node_num_keys(void *node) {
  return (uint16_t *)((char *)node + INTERNAL_NODE_NUM_KEYS_OFFSET);
}

uint32_t *internal_node_right_child(void *node) {
  return (uint32_t *)((char *)node + INTERNAL_NODE_RIGHT_CHILD_OFFSET);
}

uint32_t *internal_node_cell(void *node, uint32_t cell_num) {
  return (uint32_t *)((char *)node + INTERNAL_NODE_HEADER_SIZE +
                      cell_num * INTERNAL_NODE_CELL_SIZE);
}

uint32_t *internal_node_child(void *node, uint32_t child_num) {
  uint16_t num_keys = *internal_node_num_keys(node);
  if (child_num > num_keys) {
    printf("Tried to access child_num %d > num_keys %d\n", child_num, num_keys);
    exit(EXIT_FAILURE);
  } else if (child_num == num_keys) {
    return internal_node_right_child(node);
  } else {
    return internal_node_cell(node, child_num);
  }
}

uint32_t *internal_node_key(void *node, uint32_t key_num) {
  return (uint32_t *)((char *)internal_node_cell(node, key_num) +
                      INTERNAL_NODE_CHILD_SIZE);
}

// Node initialization
void initialize_leaf_node(void *node) {
  set_node_type(node, NODE_LEAF);
  set_node_root(node, false);
  *leaf_node_num_cells(node) = 0;
  *leaf_node_next_leaf(node) = 0; // 0 represents no sibling
}

void initialize_internal_node(void *node) {
  set_node_type(node, NODE_INTERNAL);
  set_node_root(node, false);
  *internal_node_num_keys(node) = 0;
}

// Get page function
void *get_page(Pager *pager, page_num_t page_num) {
  return pager_get_page(pager, page_num);
}

// Get unused page number
uint32_t get_unused_page_num(Pager *pager) {
  return pager_get_num_pages(pager);
}

// Get maximum key in node
uint32_t get_node_max_key(void *node) {
  switch (get_node_type(node)) {
  case NODE_INTERNAL: {
    uint16_t num_keys = *internal_node_num_keys(node);
    if (num_keys == 0)
      return 0;
    return *internal_node_key(node, num_keys - 1);
  }
  case NODE_LEAF: {
    uint16_t num_cells = *leaf_node_num_cells(node);
    if (num_cells == 0)
      return 0;
    return *leaf_node_key(node, num_cells - 1);
  }
  }
  return 0;
}

void serialize_leaf_value(void *destination, uint32_t key, void *value,
                          uint32_t value_size) {
  *(uint32_t *)destination = key;
  *(uint32_t *)((char *)destination + LEAF_NODE_KEY_SIZE) = value_size;
  memcpy((char *)destination + LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE_SIZE,
         value, value_size);
}

// Internal node operations
uint32_t internal_node_find_child(void *node, uint32_t key) {
  uint32_t num_keys = *internal_node_num_keys(node);

  // Binary search
  uint32_t min_index = 0;
  uint32_t max_index = num_keys; // there is one more child than key

  while (min_index != max_index) {
    uint32_t index = (min_index + max_index) / 2;
    uint32_t key_to_right = *internal_node_key(node, index);
    if (key_to_right >= key) {
      max_index = index;
    } else {
      min_index = index + 1;
    }
  }

  return min_index;
}

void update_internal_node_key(void *node, uint32_t old_key, uint32_t new_key) {
  uint32_t num_keys = *internal_node_num_keys(node);

  // FIXED: Search through all keys to find the one to update
  for (uint32_t i = 0; i < num_keys; i++) {
    if (*internal_node_key(node, i) == old_key) {
      *internal_node_key(node, i) = new_key;
      return;
    }
  }
}

// FIXED: Complete rewrite of internal_node_insert to maintain sorted order
void internal_node_insert(BTree *btree, page_num_t parent_page_num,
                          page_num_t child_page_num) {
  void *parent = get_page(btree->pager, parent_page_num);
  void *child = get_page(btree->pager, child_page_num);
  uint32_t child_max_key = get_node_max_key(child);

  uint32_t original_num_keys = *internal_node_num_keys(parent);

  if (original_num_keys >= INTERNAL_NODE_MAX_CELLS) {
    // Split the internal node
    internal_node_split_and_insert(btree, parent_page_num, child_page_num);
    return;
  }

  // Find the correct insertion position to maintain sorted order
  uint32_t insert_index = 0;
  while (insert_index < original_num_keys &&
         *internal_node_key(parent, insert_index) < child_max_key) {
    insert_index++;
  }

  *internal_node_num_keys(parent) = original_num_keys + 1;

  // Shift existing keys and children to make room
  for (uint32_t i = original_num_keys; i > insert_index; i--) {
    *internal_node_key(parent, i) = *internal_node_key(parent, i - 1);
    *internal_node_child(parent, i) = *internal_node_child(parent, i - 1);
  }

  // If we're inserting at the end, handle the right child specially
  if (insert_index == original_num_keys) {
    // The new child becomes the rightmost child
    *internal_node_child(parent, insert_index) =
        *internal_node_right_child(parent);
    *internal_node_key(parent, insert_index) = get_node_max_key(
        get_page(btree->pager, *internal_node_right_child(parent)));
    *internal_node_right_child(parent) = child_page_num;
  } else {
    // Insert in the middle
    *internal_node_child(parent, insert_index) = child_page_num;
    *internal_node_key(parent, insert_index) = child_max_key;
  }
}

// FIXED: Complete fix for internal node key ordering issue
void internal_node_split_and_insert(BTree *btree, page_num_t parent_page_num,
                                    page_num_t child_page_num) {
  printf("DEBUG: Splitting internal node %d, inserting child %d\n",
         parent_page_num, child_page_num);

  void *old_node = get_page(btree->pager, parent_page_num);
  void *child = get_page(btree->pager, child_page_num);

  if (!old_node || !child) {
    printf("ERROR: Failed to get pages %d or %d\n", parent_page_num,
           child_page_num);
    exit(1);
  }

  uint32_t child_max_key = get_node_max_key(child);

  // Store node state before modifications
  uint32_t old_parent = *node_parent(old_node);
  bool was_root = is_node_root(old_node);
  uint32_t old_num_keys = *internal_node_num_keys(old_node);

  printf("DEBUG: old_num_keys = %d, INTERNAL_NODE_MAX_CELLS = %d\n",
         old_num_keys, INTERNAL_NODE_MAX_CELLS);

  // Create temporary arrays to hold all keys and children
  uint32_t temp_keys[INTERNAL_NODE_MAX_CELLS + 1];
  uint32_t temp_children[INTERNAL_NODE_MAX_CELLS + 2];

  // Copy existing keys and children
  for (uint32_t i = 0; i < old_num_keys; i++) {
    temp_keys[i] = *internal_node_key(old_node, i);
    temp_children[i] = *internal_node_child(old_node, i);
  }
  temp_children[old_num_keys] = *internal_node_right_child(old_node);

  // Find correct insertion position for the new child based on key
  uint32_t insert_index = 0;
  while (insert_index < old_num_keys &&
         temp_keys[insert_index] < child_max_key) {
    insert_index++;
  }

  // Shift keys and children to make room
  for (uint32_t i = old_num_keys; i > insert_index; i--) {
    temp_keys[i] = temp_keys[i - 1];
  }
  for (uint32_t i = old_num_keys + 1; i > insert_index + 1; i--) {
    temp_children[i] = temp_children[i - 1];
  }

  // Insert new key and child
  temp_keys[insert_index] = child_max_key;
  temp_children[insert_index + 1] = child_page_num;

  uint32_t total_keys = old_num_keys + 1;
  uint32_t split_index = total_keys / 2;

  // Create new node
  page_num_t new_page_num = get_unused_page_num(btree->pager);
  void *new_node = get_page(btree->pager, new_page_num);
  initialize_internal_node(new_node);
  *node_parent(new_node) = old_parent;

  // Re-initialize old node
  initialize_internal_node(old_node);
  if (was_root) {
    set_node_root(old_node, true);
  }
  *node_parent(old_node) = old_parent;

  // Distribute to left node (old_node): keys 0 to split_index-1
  *internal_node_num_keys(old_node) = split_index;
  for (uint32_t i = 0; i < split_index; i++) {
    *internal_node_key(old_node, i) = temp_keys[i];
    *internal_node_child(old_node, i) = temp_children[i];
  }
  *internal_node_right_child(old_node) = temp_children[split_index];

  // Distribute to right node (new_node): keys split_index+1 to total_keys-1
  uint32_t right_keys = total_keys - split_index - 1;
  *internal_node_num_keys(new_node) = right_keys;
  for (uint32_t i = 0; i < right_keys; i++) {
    *internal_node_key(new_node, i) = temp_keys[split_index + 1 + i];
    *internal_node_child(new_node, i) = temp_children[split_index + 1 + i];
  }
  *internal_node_right_child(new_node) = temp_children[total_keys];

  // Update parent pointers
  for (uint32_t i = 0; i <= *internal_node_num_keys(old_node); i++) {
    uint32_t child_page = *internal_node_child(old_node, i);
    void *child_node = get_page(btree->pager, child_page);
    *node_parent(child_node) = parent_page_num;
  }

  for (uint32_t i = 0; i <= *internal_node_num_keys(new_node); i++) {
    uint32_t child_page = *internal_node_child(new_node, i);
    void *child_node = get_page(btree->pager, child_page);
    *node_parent(child_node) = new_page_num;
  }

  // The middle key gets promoted
  uint32_t promoted_key = temp_keys[split_index];

  if (was_root) {
    create_new_root(btree, new_page_num);
    void *root = get_page(btree->pager, btree->root_page_num);
    *internal_node_key(root, 0) = promoted_key;
  } else {
    // Update parent's key for old node
    void *parent = get_page(btree->pager, old_parent);
    uint32_t old_max = get_node_max_key(old_node);
    update_internal_node_key(parent, get_node_max_key(old_node), old_max);
    internal_node_insert(btree, old_parent, new_page_num);
  }
}

// Create a new root
page_num_t create_new_root(BTree *btree, page_num_t right_child_page_num) {
  void *root = get_page(btree->pager, btree->root_page_num);
  void *right_child = get_page(btree->pager, right_child_page_num);
  page_num_t left_child_page_num = get_unused_page_num(btree->pager);
  void *left_child = get_page(btree->pager, left_child_page_num);

  // Left child has data copied from old root
  memcpy(left_child, root, PAGE_SIZE);
  set_node_root(left_child, false);

  // Root node is a new internal node with one key and two children
  initialize_internal_node(root);
  set_node_root(root, true);
  *internal_node_num_keys(root) = 1;
  *internal_node_child(root, 0) = left_child_page_num;
  uint32_t left_child_max_key = get_node_max_key(left_child);
  *internal_node_key(root, 0) = left_child_max_key;
  *internal_node_right_child(root) = right_child_page_num;
  *node_parent(left_child) = btree->root_page_num;
  *node_parent(right_child) = btree->root_page_num;

  return btree->root_page_num;
}

void leaf_node_split_and_insert(BTreeCursor *cursor, uint32_t key, void *value,
                                uint32_t value_size) {
  void *old_node = get_page(cursor->btree->pager, cursor->page_num);
  uint32_t old_max_key = get_node_max_key(old_node);

  // Store important state
  uint32_t parent_page = *node_parent(old_node);
  uint32_t next_leaf = *leaf_node_next_leaf(old_node);
  bool was_root = is_node_root(old_node);
  uint32_t old_num_cells = *leaf_node_num_cells(old_node);

  // Allocate new node
  page_num_t new_page_num = get_unused_page_num(cursor->btree->pager);
  void *new_node = get_page(cursor->btree->pager, new_page_num);
  initialize_leaf_node(new_node);
  *node_parent(new_node) = parent_page;

  // Create arrays to hold all data (existing + new)
  uint32_t all_keys[LEAF_NODE_MAX_CELLS + 1];
  uint32_t all_value_sizes[LEAF_NODE_MAX_CELLS + 1];
  void *all_values[LEAF_NODE_MAX_CELLS + 1]; // Max value buffer

  // Copy existing data
  for (uint32_t i = 0; i < old_num_cells; i++) {
    all_keys[i] = *leaf_node_key(old_node, i);
    all_value_sizes[i] = *leaf_node_value_size(old_node, i);
    all_values[i] = malloc(all_value_sizes[i]);
    memcpy(all_values[i], leaf_node_value(old_node, i), all_value_sizes[i]);
  }

  // Insert new data at correct position
  uint32_t insert_pos = cursor->cell_num;
  for (uint32_t i = old_num_cells; i > insert_pos; i--) {
    all_keys[i] = all_keys[i - 1];
    all_value_sizes[i] = all_value_sizes[i - 1];
    all_values[i] = all_values[i - 1];
  }
  all_keys[insert_pos] = key;
  all_value_sizes[insert_pos] = value_size;
  all_values[insert_pos] = malloc(value_size);
  memcpy(all_values[insert_pos], value, value_size);

  uint32_t total_cells = old_num_cells + 1;
  uint32_t split_point = LEAF_NODE_LEFT_SPLIT_COUNT;

  // Re-initialize both nodes
  initialize_leaf_node(old_node);
  if (was_root) {
    set_node_root(old_node, true);
  }
  *node_parent(old_node) = parent_page;

  // Update cursor position if it ended up in the new node
  if (cursor->cell_num >= split_point) {
    cursor->page_num = new_page_num;
    cursor->cell_num = cursor->cell_num - split_point;
  }

  // Distribute cells
  for (uint32_t i = 0; i < split_point && i < total_cells; i++) {
    void *dest = leaf_node_cell(old_node, *leaf_node_num_cells(old_node));
    serialize_leaf_value(dest, all_keys[i], all_values[i], all_value_sizes[i]);
    (*leaf_node_num_cells(old_node))++;
  }

  for (uint32_t i = split_point; i < total_cells; i++) {
    void *dest = leaf_node_cell(new_node, *leaf_node_num_cells(new_node));
    serialize_leaf_value(dest, all_keys[i], all_values[i], all_value_sizes[i]);
    (*leaf_node_num_cells(new_node))++;
  }

  // FIXED: Simple and correct leaf chain linking
  // After split, old_node contains the smaller keys, new_node contains the
  // larger keys So old_node should always point to new_node
  *leaf_node_next_leaf(old_node) = new_page_num;
  *leaf_node_next_leaf(new_node) = next_leaf;

  for (uint32_t i = 0; i < total_cells; i++) {
    free(all_values[i]);
  }

  // Handle parent insertion
  if (was_root) {
    create_new_root(cursor->btree, new_page_num);
  } else {
    uint32_t new_max_key = get_node_max_key(old_node);
    void *parent = get_page(cursor->btree->pager, parent_page);
    update_internal_node_key(parent, old_max_key, new_max_key);
    internal_node_insert(cursor->btree, parent_page, new_page_num);
  }
}

// FIXED: Better cell shifting in leaf node insertion
void leaf_node_insert(BTreeCursor *cursor, uint32_t key, void *value,
                      uint32_t value_size) {
  void *node = get_page(cursor->btree->pager, cursor->page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  if (num_cells >= LEAF_NODE_MAX_CELLS) {
    leaf_node_split_and_insert(cursor, key, value, value_size);
    return;
  }

  if (cursor->cell_num < num_cells) {
    // Calculate size needed for new cell
    uint32_t new_cell_size =
        LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE_SIZE + value_size;

    // Calculate total size of cells that need to be moved
    uint32_t total_move_size = 0;
    for (uint32_t i = cursor->cell_num; i < num_cells; i++) {
      total_move_size += get_leaf_cell_size(node, i);
    }

    // Move existing cells to make room
    if (total_move_size > 0) {
      void *src = leaf_node_cell(node, cursor->cell_num);
      void *dest = (char *)src + new_cell_size;
      memmove(dest, src, total_move_size);
    }
  }

  // Insert new cell
  (*leaf_node_num_cells(node))++;
  void *destination = leaf_node_cell(node, cursor->cell_num);
  serialize_leaf_value(destination, key, value, value_size);
}

// Main B-tree operations
BTree *btree_open(Pager *pager) {
  BTree *btree = malloc(sizeof(BTree));
  btree->pager = pager;
  btree->root_page_num = 0;

  if (pager_get_num_pages(pager) == 0) {
    // New database file. Initialize page 0 as leaf node.
    void *root_node = get_page(pager, 0);
    initialize_leaf_node(root_node);
    set_node_root(root_node, true);
  }

  return btree;
}

void btree_close(BTree *btree) { free(btree); }

BTreeCursor *btree_start(BTree *btree) {
  BTreeCursor *cursor = malloc(sizeof(BTreeCursor));
  cursor->btree = btree;
  cursor->cell_num = 0;
  cursor->end_of_table = false;

  // Find the leftmost leaf node
  page_num_t page_num = btree->root_page_num;
  void *node = get_page(btree->pager, page_num);

  while (get_node_type(node) == NODE_INTERNAL) {
    // Go to the leftmost child
    page_num = *internal_node_child(node, 0);
    node = get_page(btree->pager, page_num);
  }

  cursor->page_num = page_num;

  // Check if this leaf is empty
  uint32_t num_cells = *leaf_node_num_cells(node);
  cursor->end_of_table = (num_cells == 0);

  return cursor;
}

BTreeCursor *leaf_node_find(BTree *btree, page_num_t page_num, uint32_t key) {
  void *node = get_page(btree->pager, page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  BTreeCursor *cursor = malloc(sizeof(BTreeCursor));
  cursor->btree = btree;
  cursor->page_num = page_num;
  cursor->end_of_table = false;

  // Binary search
  uint32_t min_index = 0;
  uint32_t one_past_max_index = num_cells;
  while (one_past_max_index != min_index) {
    uint32_t index = (min_index + one_past_max_index) / 2;
    uint32_t key_at_index = *leaf_node_key(node, index);
    if (key == key_at_index) {
      cursor->cell_num = index;
      return cursor;
    }
    if (key < key_at_index) {
      one_past_max_index = index;
    } else {
      min_index = index + 1;
    }
  }

  cursor->cell_num = min_index;
  return cursor;
}

BTreeCursor *internal_node_find(BTree *btree, page_num_t page_num,
                                uint32_t key) {
  void *node = get_page(btree->pager, page_num);

  uint32_t child_index = internal_node_find_child(node, key);
  uint32_t child_page_num = *internal_node_child(node, child_index);
  void *child = get_page(btree->pager, child_page_num);
  switch (get_node_type(child)) {
  case NODE_LEAF:
    return leaf_node_find(btree, child_page_num, key);
  case NODE_INTERNAL:
    return internal_node_find(btree, child_page_num, key);
  }
  // This should never be reached, but adding for safety
  return NULL;
}

BTreeCursor *btree_find(BTree *btree, uint32_t key) {
  page_num_t root_page_num = btree->root_page_num;
  void *root_node = get_page(btree->pager, root_page_num);

  if (get_node_type(root_node) == NODE_LEAF) {
    return leaf_node_find(btree, root_page_num, key);
  } else {
    return internal_node_find(btree, root_page_num, key);
  }
}

int btree_insert(BTree *btree, uint32_t key, void *value, uint32_t value_size) {
  BTreeCursor *cursor = btree_find(btree, key);

  void *node = get_page(btree->pager, cursor->page_num);
  uint32_t num_cells = *leaf_node_num_cells(node);

  if (cursor->cell_num < num_cells) {
    uint32_t key_at_index = *leaf_node_key(node, cursor->cell_num);
    if (key_at_index == key) {
      free(cursor);
      return -1; // Key already exists
    }
  }

  leaf_node_insert(cursor, key, value, value_size);
  free(cursor);
  return 0;
}

void btree_cursor_advance(BTreeCursor *cursor) {
  page_num_t page_num = cursor->page_num;
  void *node = get_page(cursor->btree->pager, page_num);

  cursor->cell_num += 1;
  if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
    // Advance to next leaf node
    uint32_t next_page_num = *leaf_node_next_leaf(node);
    if (next_page_num == 0) {
      // This was rightmost leaf
      cursor->end_of_table = true;
    } else {
      cursor->page_num = next_page_num;
      cursor->cell_num = 0;
    }
  }
}

void btree_cursor_get_value(BTreeCursor *cursor, void *value_buffer,
                            uint32_t buffer_size, uint32_t *value_size) {
  page_num_t page_num = cursor->page_num;
  void *node = get_page(cursor->btree->pager, page_num);

  uint32_t size_of_value = *leaf_node_value_size(node, cursor->cell_num);
  *value_size = size_of_value; // Always report the actual size

  // Only copy if the provided buffer is large enough
  if (buffer_size >= size_of_value) {
    void *src = leaf_node_value(node, cursor->cell_num);
    memcpy(value_buffer, src, size_of_value);
  }
}