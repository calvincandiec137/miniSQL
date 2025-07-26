#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "btree.h"
#include "common.h"

// Defines the structure of a B-Tree node page.
// It follows the layout from the design document:
// [Header | Cell Offset Array -> | Free Space | <- Cell Content]

typedef enum { NODE_INTERNAL, NODE_LEAF } NodeType;

// Common Node Header Layout
const uint32_t NODE_TYPE_SIZE = sizeof(uint8_t);
const uint32_t NODE_TYPE_OFFSET = 0;
const uint32_t IS_ROOT_SIZE = sizeof(uint8_t);
const uint32_t IS_ROOT_OFFSET = NODE_TYPE_SIZE;
const uint32_t PARENT_POINTER_SIZE = sizeof(uint32_t);
const uint32_t PARENT_POINTER_OFFSET = IS_ROOT_OFFSET + IS_ROOT_SIZE;
const uint32_t COMMON_NODE_HEADER_SIZE = NODE_TYPE_SIZE + IS_ROOT_SIZE + PARENT_POINTER_SIZE;

// Leaf Node Header Layout
const uint32_t LEAF_NODE_NUM_CELLS_SIZE = sizeof(uint16_t);
const uint32_t LEAF_NODE_NUM_CELLS_OFFSET = COMMON_NODE_HEADER_SIZE;
const uint32_t LEAF_NODE_CONTENT_START_SIZE = sizeof(uint16_t);
const uint32_t LEAF_NODE_CONTENT_START_OFFSET = LEAF_NODE_NUM_CELLS_OFFSET + LEAF_NODE_NUM_CELLS_SIZE;
const uint32_t LEAF_NODE_HEADER_SIZE = COMMON_NODE_HEADER_SIZE + LEAF_NODE_NUM_CELLS_SIZE + LEAF_NODE_CONTENT_START_SIZE;

// Leaf Node Cell Layout (stored at the end of the page)
const uint32_t LEAF_NODE_KEY_SIZE = sizeof(uint32_t);
const uint32_t LEAF_NODE_VALUE_SIZE = sizeof(uint32_t);
// A cell stores: [key | value_size | value_payload]
#define size_of_attribute(Struct, Attribute) sizeof(((Struct*)0)->Attribute)

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


// Page/Node accessor functions
void* get_page(Pager* pager, page_num_t page_num) { return pager_get_page(pager, page_num); }
uint16_t* leaf_node_num_cells(void* node) { return (uint16_t*)((char*)node + LEAF_NODE_NUM_CELLS_OFFSET); }
uint16_t* leaf_node_content_start(void* node) { return (uint16_t*)((char*)node + LEAF_NODE_CONTENT_START_OFFSET); }
uint16_t* leaf_node_cell_offset(void* node, uint32_t cell_num) { return (uint16_t*)((char*)node + LEAF_NODE_HEADER_SIZE + cell_num * sizeof(uint16_t)); }

void* leaf_node_cell_data(void* node, uint32_t cell_num) {
    uint16_t offset = *leaf_node_cell_offset(node, cell_num);
    return (void*)((char*)node + offset);
}

uint32_t* leaf_node_key(void* node, uint32_t cell_num) {
    return (uint32_t*)leaf_node_cell_data(node, cell_num);
}

void* leaf_node_value(void* node, uint32_t cell_num) {
    void* cell = leaf_node_cell_data(node, cell_num);
    return (void*)((char*)cell + LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE);
}

uint32_t* leaf_node_value_size(void* node, uint32_t cell_num) {
    void* cell = leaf_node_cell_data(node, cell_num);
    return (uint32_t*)((char*)cell + LEAF_NODE_KEY_SIZE);
}

void initialize_leaf_node(void* node) {
    *(NodeType*)((char*)node + NODE_TYPE_OFFSET) = NODE_LEAF;
    *leaf_node_num_cells(node) = 0;
    *leaf_node_content_start(node) = PAGE_SIZE; // Content starts at the end
}

BTree* btree_open(Pager* pager) {
    BTree* btree = malloc(sizeof(BTree));
    btree->pager = pager;
    btree->root_page_num = 0;

    if (pager_get_num_pages(pager) == 0) {
        void* root_node = get_page(pager, 0);
        initialize_leaf_node(root_node);
    }
    return btree;
}

void btree_close(BTree* btree) { free(btree); }

BTreeCursor* btree_find(BTree* btree, uint32_t key);
void leaf_node_insert(BTreeCursor* cursor, uint32_t key, void* value, uint32_t value_size);

int btree_insert(BTree* btree, uint32_t key, void* value, uint32_t value_size) {
    // For now, we only handle the single-node leaf case.
    // A full implementation would need to find the correct leaf node.
    BTreeCursor* cursor = btree_find(btree, key);

    void* node = get_page(btree->pager, cursor->page_num);
    uint32_t num_cells = *leaf_node_num_cells(node);
    
    // Check for space BEFORE attempting insert
    uint32_t cell_data_size = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE + value_size;
    uint16_t free_space = *leaf_node_content_start(node) - (LEAF_NODE_HEADER_SIZE + (num_cells * sizeof(uint16_t)));
    if (cell_data_size + sizeof(uint16_t) > free_space) {
        // For this assignment, we assume no node splitting is needed.
        // In a full DB, you'd split the node here.
        free(cursor);
        return -1; // Out of space
    }

    leaf_node_insert(cursor, key, value, value_size);
    free(cursor);
    return 0;
}

void leaf_node_insert(BTreeCursor* cursor, uint32_t key, void* value, uint32_t value_size) {
    void* node = get_page(cursor->btree->pager, cursor->page_num);
    uint32_t cell_num_to_insert = cursor->cell_num;
    
    uint16_t* num_cells = leaf_node_num_cells(node);
    
    // Make space in the offset array for the new cell's offset
    for (uint32_t i = *num_cells; i > cell_num_to_insert; i--) {
        *leaf_node_cell_offset(node, i) = *leaf_node_cell_offset(node, i - 1);
    }
    
    // Write the cell data (key, value size, value payload) to the page
    uint32_t cell_data_size = LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE + value_size;
    uint16_t* content_start = leaf_node_content_start(node);
    *content_start -= cell_data_size;
    
    void* cell_storage_location = (char*)node + *content_start;
    *(uint32_t*)(cell_storage_location) = key;
    *(uint32_t*)((char*)cell_storage_location + LEAF_NODE_KEY_SIZE) = value_size;
    memcpy((char*)cell_storage_location + LEAF_NODE_KEY_SIZE + LEAF_NODE_VALUE_SIZE, value, value_size);
    
    // Write the new cell's offset into the offset array
    *leaf_node_cell_offset(node, cell_num_to_insert) = *content_start;
    
    *num_cells += 1;
}


BTreeCursor* btree_find(BTree* btree, uint32_t key) {
    // For now, assume root is the only node and it's a leaf
    void* root_node = get_page(btree->pager, btree->root_page_num);
    
    BTreeCursor* cursor = malloc(sizeof(BTreeCursor));
    cursor->btree = btree;
    cursor->page_num = btree->root_page_num;
    cursor->end_of_table = false;

    uint32_t num_cells = *leaf_node_num_cells(root_node);
    if (num_cells == 0) {
        cursor->cell_num = 0;
        return cursor;
    }
    
    // Binary search to find insertion point or matching key
    uint32_t min_index = 0;
    uint32_t one_past_max_index = num_cells;
    while (one_past_max_index != min_index) {
        uint32_t index = (min_index + one_past_max_index) / 2;
        uint32_t key_at_index = *leaf_node_key(root_node, index);
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


void btree_cursor_advance(BTreeCursor* cursor) {
    void* node = get_page(cursor->btree->pager, cursor->page_num);
    cursor->cell_num++;
    if (cursor->cell_num >= (*leaf_node_num_cells(node))) {
        cursor->end_of_table = true;
    }
}

void btree_cursor_get_value(BTreeCursor* cursor, void** value, uint32_t* value_size) {
    void* node = get_page(cursor->btree->pager, cursor->page_num);
    *value_size = *leaf_node_value_size(node, cursor->cell_num);
    void* value_payload = leaf_node_value(node, cursor->cell_num);
    
    // Return a copy of the data, so the caller can manage its memory.
    *value = malloc(*value_size);
    memcpy(*value, value_payload, *value_size);
}