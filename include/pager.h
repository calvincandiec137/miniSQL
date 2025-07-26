#ifndef PAGER_H
#define PAGER_H

#include "common.h"

typedef struct Pager Pager;

Pager* pager_open(const char* filename);
void* pager_get_page(Pager* pager, page_num_t page_num);
void pager_mark_dirty(Pager* pager, page_num_t page_num);
void pager_flush_page(Pager* pager, page_num_t page_num);
void pager_close(Pager* pager);
uint32_t pager_get_num_pages(Pager* pager);

#endif