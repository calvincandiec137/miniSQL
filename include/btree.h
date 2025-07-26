#ifndef BTREE_H
#define BTREE_H

#include "pager.h"
#include <stdint.h>

typedef struct BTree BTree;
typedef struct BTreeCursor BTreeCursor;

BTree* btree_open(Pager* pager);
void btree_close(BTree* btree);

int btree_insert(BTree* btree, uint32_t key, void* value, uint32_t value_size);

BTreeCursor* btree_find(BTree* btree, uint32_t key);
void btree_cursor_advance(BTreeCursor* cursor);
void btree_cursor_get_value(BTreeCursor* cursor, void** value, uint32_t* value_size);

#endif