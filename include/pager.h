#ifndef PAGER_H
#define PAGER_H

#include "common.h"

#define TABLE_MAX_PAGES 100

typedef struct {
    int file_descriptor;
    uint32_t file_length;
    void* pages[TABLE_MAX_PAGES];
} Pager;

Pager* pager_open(const char* filename);
void* pager_get_page(Pager* pager, page_num_t page_num);
void pager_flush(Pager* pager, uint32_t page_num, uint32_t size);
void pager_close(Pager* pager);
uint32_t pager_get_num_pages(Pager* pager);

#endif